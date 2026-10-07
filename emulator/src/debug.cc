#include <debug.hh>
#include <host_input.hh>
#include "../../disassembler/include/disassembler.hh"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#ifdef __unix__
#include <poll.h>
#include <unistd.h>
#include <cerrno>
#include <termios.h>
#endif

namespace
{
    std::string hex(unsigned value, unsigned width = 4)
    {
        std::ostringstream out;
        out << "0x" << std::hex << std::setw(width) << std::setfill('0') << value;
        return out.str();
    }
    std::string lower(std::string text)
    {
        for (char &ch : text)
            ch = std::tolower(static_cast<unsigned char>(ch));
        return text;
    }
    unsigned number(const std::string &text, unsigned maximum = 65535)
    {
        if (text.empty() || text[0] == '-' || text[0] == '+')
            throw std::invalid_argument("Expected a nonnegative number");
        size_t used = 0;
        // Numbers are decimal or 0x-prefixed; bare hexadecimal with letters is
        // accepted too, so a pasted PC such as ff00 works.
        const bool bare_hex = text.find_first_of("abcdefABCDEF") != std::string::npos && !text.starts_with("0x") && !text.starts_with("0X");
        auto value = std::stoull(text, &used, (bare_hex || text.starts_with("0x") || text.starts_with("0X")) ? 16 : 10);
        if (used != text.size() || value > maximum)
            throw std::out_of_range("Invalid number or value out of range: " + text);
        return value;
    }
    unsigned address(const std::string &expression, const ANC216::CPUInfo &info)
    {
        auto text = lower(expression);
        const auto separator = text.find_first_of("+-", 1);
        const auto base = text.substr(0, separator);
        unsigned result;
        if (base == "pc")
            result = info.pc;
        else if (base == "sp")
            result = info.sp;
        else if (base == "bp")
            result = info.bp;
        else if (base.size() == 2 && (base[0] == 'r' || base[0] == 'l') && base[1] >= '0' && base[1] <= '7')
            result = uint16_t(info.reg[base[1] - '0']) & (base[0] == 'l' ? 255 : 65535);
        else
            result = number(base);
        if (separator != std::string::npos)
        {
            const unsigned delta = number(text.substr(separator + 1));
            if (text[separator] == '+' && delta > 65535 - result)
                throw std::out_of_range("Address overflow");
            if (text[separator] == '-' && delta > result)
                throw std::out_of_range("Address underflow");
            result = text[separator] == '+' ? result + delta : result - delta;
        }
        return result;
    }
    unsigned instruction_size(ANC216::CPU &cpu, unsigned at)
    {
        auto bytes = cpu.memory(at, std::min(6u, MAX_MEM - at));
        auto mode = anc216_isa::decode(bytes[0]);
        if (bytes.size() < 2 || !anc216_isa::valid(bytes[1], mode) || 2 + mode.size > bytes.size())
            return 1;
        return 2 + mode.size;
    }
    void disassemble(ANC216::CPU &cpu, unsigned at, unsigned count)
    {
        auto breakpoints = cpu.list_breakpoints();
        const auto pc = cpu.get_pc();
        for (unsigned i = 0; i < count && at < MAX_MEM; ++i)
        {
            const auto length = instruction_size(cpu, at);
            auto bytes = cpu.memory(at, length);
            ANC216::Disassembler decoder(bytes);
            auto text = decoder.disassemble();
            if (!text.empty() && text.front() == '\t')
                text.erase(0, 1);
            if (!text.empty() && text.back() == '\n')
                text.pop_back();
            std::cout << (at == pc ? "=> " : "   ")
                      << (breakpoints.contains(at) && breakpoints[at] ? "* " : "  ")
                      << hex(at) << "  ";
            std::ostringstream raw;
            for (auto byte : bytes)
                raw << hex(byte, 2).substr(2) << ' ';
            std::cout << std::left << std::setw(19) << raw.str() << std::right << text << '\n';
            at += length;
        }
    }
    void memory(ANC216::CPU &cpu, unsigned at, unsigned size)
    {
        auto bytes = cpu.memory(at, size);
        for (size_t i = 0; i < bytes.size(); i += 16)
        {
            std::cout << hex(at + i) << "  ";
            for (size_t column = 0; column < 16; ++column)
                std::cout << (i + column < bytes.size() ? hex(bytes[i + column], 2).substr(2) + " " : "   ");
            std::cout << " |";
            for (size_t column = i; column < std::min(i + 16, bytes.size()); ++column)
                std::cout << (bytes[column] >= 32 && bytes[column] <= 126 ? char(bytes[column]) : '.');
            std::cout << "|\n";
        }
    }
    void context(ANC216::CPU &cpu)
    {
        const auto state = cpu.debug_state();
        std::cout << (state.halted ? "Halted" : state.running ? "Running"
                                                              : "Paused")
                  << " after " << state.instructions << " instructions";
        if (state.breakpoint)
            std::cout << (state.temporary ? " (target reached at " : " (breakpoint at ") << hex(*state.breakpoint) << ')';
        if (!cpu.error().empty())
            std::cout << ": " << cpu.error();
        std::cout << '\n';
        disassemble(cpu, cpu.get_pc(), 1);
    }

    class Terminal
    {
#ifdef __unix__
        struct termios original{};
#endif
    public:
        bool interactive = false;
        Terminal()
        {
#ifdef __unix__
            if (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO) && tcgetattr(STDIN_FILENO, &original) == 0)
            {
                auto mode = original;
                mode.c_lflag &= ~(ICANON | ECHO);
                mode.c_cc[VMIN] = 1;
                mode.c_cc[VTIME] = 0;
                interactive = tcsetattr(STDIN_FILENO, TCSANOW, &mode) == 0;
            }
#endif
        }
        ~Terminal()
        {
#ifdef __unix__
            if (interactive)
                tcsetattr(STDIN_FILENO, TCSANOW, &original);
#endif
        }
        void redraw(const std::string &line, size_t cursor) const
        {
            if (!interactive)
                return;
            std::cout << "\r\033[2K(anc216) " << line;
            if (cursor < line.size())
                std::cout << "\033[" << line.size() - cursor << 'D';
            std::cout << std::flush;
        }
    };

    // SDL, stop notifications and terminal editing share the main thread.
    bool read_line(std::string &line, ANC216::CPU &cpu, ANC216::EmemMapper &mapper,
                   ANC216::Video::Window &window, uint64_t &last_stop, bool &last_halt,
                   Terminal &terminal, const std::vector<std::string> &history)
    {
#ifdef __unix__
        line.clear();
        size_t cursor = 0, history_index = history.size();
        std::string draft, escape;
        for (;;)
        {
            if (ANC216::host_shutdown_requested)
            {
                // In the debugger SIGINT pauses; do not queue a shutdown pin.
                ANC216::host_shutdown_requested = 0;
                cpu.stop();
                std::cout << '\n';
                context(cpu);
                std::cout << "(anc216) " << line << std::flush;
                terminal.redraw(line, cursor);
            }
            if (!window.poll() || (window.close_requested() && cpu.halted()))
                return false;
            mapper.present();
            const auto state = cpu.debug_state();
            if (state.stop_serial != last_stop || (state.halted && !last_halt))
            {
                last_stop = state.stop_serial;
                last_halt = state.halted;
                std::cout << '\n';
                context(cpu);
                std::cout << "(anc216) " << line << std::flush;
                terminal.redraw(line, cursor);
            }
            struct pollfd input = {STDIN_FILENO, POLLIN, 0};
            const int ready = poll(&input, 1, 20);
            if (ready < 0)
            {
                if (errno == EINTR)
                    continue;
                return false;
            }
            if (!ready)
                continue;
            char ch;
            if (read(STDIN_FILENO, &ch, 1) != 1)
                return !line.empty();
            if (ch == '\n' || ch == '\r')
            {
                if (terminal.interactive)
                    std::cout << '\n';
                return true;
            }
            if (!terminal.interactive)
            {
                line += ch;
                continue;
            }
            if (!escape.empty())
            {
                escape += ch;
                if (escape == "\033[")
                    continue;
                if (escape.size() > 2 && (std::isdigit(static_cast<unsigned char>(ch)) || ch == ';'))
                    continue;
                if (escape == "\033[A" || escape == "\033[B")
                {
                    if (history_index == history.size())
                        draft = line;
                    if (escape.back() == 'A' && history_index)
                        --history_index;
                    else if (escape.back() == 'B' && history_index < history.size())
                        ++history_index;
                    line = history_index == history.size() ? draft : history[history_index];
                    cursor = line.size();
                }
                else if (escape == "\033[D" && cursor)
                    --cursor;
                else if (escape == "\033[C" && cursor < line.size())
                    ++cursor;
                else if (escape == "\033[H" || escape == "\033[1~")
                    cursor = 0;
                else if (escape == "\033[F" || escape == "\033[4~")
                    cursor = line.size();
                else if (escape == "\033[3~" && cursor < line.size())
                    line.erase(cursor, 1);
                escape.clear();
            }
            else if (ch == 27)
            {
                escape = ch;
                continue;
            }
            else if (ch == 4)
            {
                if (line.empty())
                {
                    std::cout << '\n';
                    return false;
                }
                if (cursor < line.size())
                    line.erase(cursor, 1);
            }
            else if (ch == 127 || ch == 8)
            {
                if (cursor)
                    line.erase(--cursor, 1);
            }
            else if (ch == 1)
                cursor = 0;
            else if (ch == 5)
                cursor = line.size();
            else if (ch == 21)
            {
                line.clear();
                cursor = 0;
            }
            else if (ch == 12)
                std::cout << "\033[2J\033[H";
            else if (ch == '\t' && line.find(' ') == std::string::npos)
            {
                static const std::vector<std::string> commands = {
                    "break", "clear", "continue", "delete", "devices", "disable", "disasm", "enable", "exit",
                    "finish", "help", "history", "mem", "next", "quit", "regs", "reset", "set",
                    "shutdown", "soft-reset", "stack", "status", "step", "stop", "until"};
                std::vector<std::string> matches;
                for (const auto &command : commands)
                    if (command.starts_with(line))
                        matches.push_back(command);
                if (matches.size() == 1)
                    line = matches[0] + " ";
                else if (!matches.empty())
                {
                    std::cout << '\n';
                    for (const auto &match : matches)
                        std::cout << match << "  ";
                    std::cout << '\n';
                }
                cursor = line.size();
            }
            else if (static_cast<unsigned char>(ch) >= 32 && static_cast<unsigned char>(ch) < 127)
                line.insert(cursor++, 1, ch);
            terminal.redraw(line, cursor);
        }
#else
        (void)mapper;
        (void)last_stop;
        (void)last_halt;
        (void)terminal;
        (void)history;
        window.poll();
        return bool(std::getline(std::cin, line));
#endif
    }

} // namespace

void show_cpu_info(ANC216::CPUInfo info)
{
    for (unsigned i = 0; i < 8; ++i)
        std::cout << "R" << i << "=" << hex(uint16_t(info.reg[i])).substr(2) << (i == 7 ? '\n' : ' ');
    std::cout << "PC=" << hex(info.pc).substr(2) << " SP=" << hex(info.sp).substr(2)
              << " BP=" << hex(info.bp).substr(2) << " SR=" << hex(info.sr, 2).substr(2);
    std::cout << " [";
    const char names[] = {'N', 'O', 'I', 'T', 'S', 'A', 'Z', 'C'};
    for (unsigned bit = 0; bit < 8; ++bit)
        std::cout << (info.sr & (0x80 >> bit) ? names[bit] : '-');
    std::cout << "] last=" << hex(info.current_instruction) << '\n'
              << "MTU IMEM=" << hex(info.mtu[0]).substr(2) << ".." << hex(info.mtu[1]).substr(2)
              << " EMEM=" << hex(info.mtu[2]).substr(2) << ".." << hex(info.mtu[3]).substr(2)
              << " STACK=" << hex(info.mtu[4]).substr(2) << ".." << hex(info.mtu[5]).substr(2) << '\n';
}

void print_debug_help()
{
    std::cout << "Execution: c/continue/start, stop/pause, s/step/ni [count], n/next, until <addr>, finish\n"
                 "Breakpoints: b/break <addr>, bl/break list, delete <addr>, enable/disable <addr>, clear\n"
                 "Inspect: r/regs/sh info, status, u/disasm [addr] [count], x/mem <addr> [bytes], stack [bytes]\n"
                 "Devices: devices (connected EMEM addresses, types and IDs)\n"
        "Edit: set <r0..r7|l0..l7|pc|sp|bp|sr> <value>, set mem <addr> <byte>\n"
                 "Control: reset (CPU hard reset), soft-reset, shutdown, q/quit/exit\n"
                 "Terminal: arrows edit/history, Home/End/Delete, Tab command completion, Ctrl+U clear line\n"
                 "Other: help/?, history, !<history number>; empty line repeats step/next/inspection\n"
                 "Addresses are physical IMEM. Numbers: decimal or 0x hex; pc/sp/bp/rN/lN +/- offset accepted.\n"
                 "Continue runs in the background; use stop to pause. Terminal Ctrl+C pauses in debug mode.\n"
                 "SDL Ctrl+D/C and debugger soft-reset/shutdown still request guest control pins.\n";
}

void debug_console(ANC216::CPU &cpu, ANC216::EmemMapper &mapper, ANC216::Video::Window &window)
{
    Terminal terminal;
    cpu.stop();
    std::cout << "ANC216 debugger. Type help for commands.\n";
    context(cpu);
    std::vector<std::string> history;
    std::string repeat, command;
    auto initial = cpu.debug_state();
    uint64_t last_stop = initial.stop_serial;
    bool last_halt = initial.halted;
    auto show = [&]
    {
        context(cpu);
        auto state = cpu.debug_state();
        last_stop = state.stop_serial;
        last_halt = state.halted;
    };
    while (std::cout << "(anc216) " << std::flush, read_line(command, cpu, mapper, window, last_stop, last_halt, terminal, history))
    {
        try
        {
            const auto first = command.find_first_not_of(" \t");
            command = first == std::string::npos ? "" : command.substr(first, command.find_last_not_of(" \t") - first + 1);
            if (command.empty())
            {
                if (repeat.empty())
                    continue;
                command = repeat;
            }
            if (command.starts_with("!"))
            {
                const auto index = number(command.substr(1), history.size());
                if (!index)
                    throw std::invalid_argument("History numbers start at 1");
                command = history[index - 1];
                std::cout << command << '\n';
            }
            history.push_back(command);
            std::istringstream input(command);
            std::vector<std::string> args;
            for (std::string word; input >> word;)
                args.push_back(word);
            if (args.empty())
                continue;
            auto op = lower(args[0]);
            if (op == "sh" && args.size() == 2 && args[1] == "info")
            {
                op = "regs";
                args = {"regs"};
            }
            if (op == "imem" && args.size() >= 2 && args[1] == "watch")
            {
                op = "mem";
                args.erase(args.begin());
                args[0] = "mem";
            }
            auto arity = [&](size_t minimum, size_t maximum)
            {
                if (args.size() < minimum + 1 || args.size() > maximum + 1)
                    throw std::invalid_argument("Wrong number of arguments; type help");
            };
            auto value = [&](size_t index)
            {
                return address(args.at(index), cpu.get_info());
            };
            const bool repeatable = op == "s" || op == "step" || op == "si" || op == "ni" || op == "n" || op == "next" ||
                                    op == "r" || op == "regs" || op == "status" || op == "u" || op == "disasm" || op == "x" || op == "mem" || op == "stack";
            if (repeatable)
                repeat = command;
            if (op == "q" || op == "quit" || op == "exit")
            {
                arity(0, 0);
                break;
            }
            else if (op == "help" || op == "h" || op == "?")
            {
                arity(0, 0);
                print_debug_help();
            }
            else if (op == "c" || op == "continue" || op == "start")
            {
                arity(0, 0);
                if (cpu.halted())
                    throw std::runtime_error("CPU halted; reset before continuing");
                cpu.start();
                std::cout << "Running; stop or Ctrl+C pauses.\n";
            }
            else if (op == "stop" || op == "pause")
            {
                arity(0, 0);
                cpu.stop();
                show();
            }
            else if (op == "s" || op == "step" || op == "si" || op == "ni")
            {
                arity(0, 1);
                const unsigned count = args.size() == 2 ? number(args[1], 100000) : 1;
                if (!count)
                    throw std::invalid_argument("Step count must be positive");
                for (unsigned i = 0; i < count && !cpu.halted(); ++i)
                {
                    cpu.step();
                    if (i % 1024 == 0)
                    {
                        window.poll();
                        mapper.present();
                    }
                }
                show_cpu_info(cpu.get_info());
                show();
            }
            else if (op == "n" || op == "next")
            {
                arity(0, 0);
                cpu.stop();
                const auto info = cpu.get_info();
                const auto bytes = cpu.memory(info.pc, std::min(4u, MAX_MEM - unsigned(info.pc)));
                if ((bytes.size() == 4 && bytes[0] == 0x80 && bytes[1] == 4) ||
                    (bytes.size() >= 2 && bytes[0] == 0 && bytes[1] == 3))
                {
                    const auto target = uint16_t(info.pc + (bytes[1] == 4 ? 4 : 2));
                    cpu.run_until(target, info.sp);
                    std::cout << "Running to return " << hex(target) << "; stop or Ctrl+C pauses.\n";
                }
                else
                {
                    cpu.step();
                    show();
                }
            }
            else if (op == "until" || op == "finish")
            {
                arity(op == "until" ? 1 : 0, op == "until" ? 1 : 0);
                cpu.stop();
                const auto info = cpu.get_info();
                if (op == "until")
                    cpu.run_until(value(1));
                else
                {
                    if (info.bp < 3)
                        throw std::runtime_error("BP does not contain a CALL frame");
                    auto frame = cpu.memory(info.bp - 3, 3);
                    cpu.run_until((unsigned(frame[0]) << 8) | frame[1], info.bp - 3);
                }
                std::cout << "Running to target; stop or Ctrl+C pauses.\n";
            }
            else if (op == "r" || op == "regs")
            {
                arity(0, 0);
                show_cpu_info(cpu.get_info());
                show();
            }
            else if (op == "devices")
            {
                arity(0, 0);
                unsigned count = 0;
                std::cout << "EMEM address/range  Device       ID\n";
                for (unsigned at = 0; at < MAX_MEM; ++at)
                {
                    const auto id = mapper.info_req(at);
                    if (id == 0xffff)
                        continue;
                    const char *name = id == ANC216::ROM ? "ROM" :
                                       id == ANC216::KEYBOARD ? "Keyboard" :
                                       id == ANC216::MPME_CARD ? "MPME216" :
                                       id == ANC216::AVC64_VIDEO_CARD ? "AVC64" : "Unknown";
                    const unsigned begin = at;
                    if (id == ANC216::ROM)
                        while (at + 1 < MAX_MEM && mapper.is_rom(at + 1))
                            ++at;
                    const std::string range = hex(begin) + (at != begin ? "-" + hex(at) : "");
                    std::cout << std::left << std::setw(20) << range << std::setw(13) << name
                              << std::right << hex(id) << '\n';
                    ++count;
                }
                if (!count)
                    std::cout << "No devices connected.\n";
            }
            else if (op == "status")
            {
                arity(0, 0);
                show();
            }
            else if (op == "u" || op == "disasm")
            {
                arity(0, 2);
                const unsigned at = args.size() >= 2 ? value(1) : cpu.get_pc();
                const unsigned count = args.size() == 3 ? number(args[2], 1000) : 8;
                if (!count)
                    throw std::invalid_argument("Instruction count must be positive");
                disassemble(cpu, at, count);
            }
            else if (op == "x" || op == "mem" || op == "stack")
            {
                arity(op == "stack" ? 0 : 1, op == "stack" ? 1 : 2);
                const auto info = cpu.get_info();
                const unsigned at = op == "stack" ? (info.sp > 32 ? info.sp - 32 : 0) : value(1);
                const unsigned index = op == "stack" ? 1 : 2;
                const unsigned size = args.size() > index ? number(args[index], MAX_MEM) : std::min(64u, MAX_MEM - at);
                memory(cpu, at, size);
            }
            else if (op == "b" || op == "br" || op == "brk" || op == "break" || op == "bl")
            {
                arity(0, 1);
                if (args.size() == 1 || args[1] == "list")
                {
                    auto points = cpu.list_breakpoints();
                    if (points.empty())
                        std::cout << "No breakpoints.\n";
                    for (auto [at, enabled] : points)
                    {
                        std::cout << (enabled ? "[enabled] " : "[disabled] ") << hex(at) << '\n';
                        disassemble(cpu, at, 1);
                    }
                }
                else
                {
                    const unsigned at = value(1);
                    cpu.set_breakpoint(at);
                    std::cout << "Breakpoint set at " << hex(at) << '\n';
                }
            }
            else if (op == "delete" || op == "db" || op == "enable" || op == "disable")
            {
                arity(1, 1);
                const auto at = value(1);
                if (!cpu.list_breakpoints().contains(at))
                    throw std::runtime_error("No breakpoint at " + hex(at));
                if (op == "delete" || op == "db")
                    cpu.remove_breakpoint(at);
                else
                    cpu.set_breakpoint(at, op == "enable");
                std::cout << "Breakpoint " << op << " at " << hex(at) << '\n';
            }
            else if (op == "clear")
            {
                arity(0, 0);
                cpu.clear_breakpoints();
                std::cout << "All breakpoints cleared.\n";
            }
            else if (op == "set")
            {
                arity(2, 3);
                if (lower(args[1]) == "mem")
                {
                    arity(3, 3);
                    const auto at = value(2);
                    const auto byte = number(args[3], 255);
                    cpu.stop();
                    cpu.poke(at, byte);
                }
                else
                {
                    arity(2, 2);
                    cpu.debug_set(lower(args[1]), value(2));
                }
                show();
            }
            else if (op == "reset")
            {
                arity(0, 0);
                cpu.load_init_state();
                show();
            }
            else if (op == "soft-reset" || op == "shutdown")
            {
                arity(0, 0);
                if (op == "soft-reset")
                    cpu.request_soft_reset();
                else
                    cpu.request_shutdown();
                cpu.start();
            }
            else if (op == "history")
            {
                arity(0, 0);
                for (size_t i = 0; i < history.size(); ++i)
                    std::cout << i + 1 << "  " << history[i] << '\n';
            }
            else
                throw std::invalid_argument("Unknown command '" + args[0] + "'. Type help or ?.");
        }
        catch (const std::exception &error)
        {
            std::cout << "Error: " << error.what() << '\n';
        }
        mapper.present();
    }
    cpu.shutdown();
    cpu.wait();
}
