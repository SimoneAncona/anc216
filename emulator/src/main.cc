#include <cpu.hh>
#include <emem.hh>
#include <video.hh>
#include <debug.hh>
#include <host_input.hh>
#include <pacing.hh>
#include <cmath>
#include <iostream>
#include <string>
#include <stdexcept>
#include <thread>
#include <chrono>

static void help(const char *name)
{
    std::cout << "Usage: " << name << " --boot <raw ROM image> [options]\n"
                                      "  -b, --boot <file>             Load up to 256 bytes at 0xff00\n"
                                      "  -d, --debug                   Interactive debugger\n"
                                      "  --novideo --noaudio --nokeyboard\n"
                                      "  --zoom=<1..16>                Window scale (default 5: 1280x1120)\n"
                                      "  --uncapped                    Maximum CPU speed, keeping video enabled\n"
                                      "  --speed=<positive number>     Instruction rate multiplier\n"
                                      "  -i, --insert <address> <file> Attach a raw ROM device\n"
                                      "  --insert-card <address> <file> Attach a writable MPME device\n"
                                      "  --insert-charmap <file>        Load AVC64 character/texture map\n"
                                      "  --gpu=default                 AVC64 (SDL build only)\n"
                                      "  Terminal: Ctrl+C requests the guest shutdown pin\n"
                                      "  SDL: Ctrl+D soft reset; Ctrl+C or close window requests shutdown\n"
                                      "  --max-cycles=<number>         Stop after this many instructions\n";
}
int main(int argc, char **argv)
{
    try
    {
        ANC216::EmuFlags flags;
#ifndef ANC216_WITH_SDL
        flags.novideo = true;
#endif
        uint64_t limit = 0;
        auto next = [&](int &i) -> std::string
        {
            if (++i >= argc)
                throw std::runtime_error("Missing option argument");
            return argv[i];
        };
        auto number = [](const std::string &s)
        {
            size_t used = 0;
            auto n = std::stoull(s, &used, 0);
            if (used != s.size() || s.empty() || s[0] == '-')
                throw std::runtime_error("Invalid number: " + s);
            return n;
        };
        for (int i = 1; i < argc; ++i)
        {
            const std::string arg = argv[i];
            if (arg == "--help")
            {
                help(argv[0]);
                return 0;
            }
            else if (arg == "-b" || arg == "--boot")
                flags.bootfile = next(i);
            else if (arg == "--insert-charmap")
                flags.charmap = next(i);
            else if (arg == "-d" || arg == "--debug")
                flags.debug_mode = true;
            else if (arg == "--novideo")
                flags.novideo = true;
            else if (arg == "--noaudio")
                flags.noaudio = true;
            else if (arg == "--nokeyboard")
                flags.nokeyboard = true;
            else if (arg == "--uncapped")
                flags.uncapped = true;
            else if (arg == "-f" || arg == "--fast-mode")
            {
                flags.fast_mode = true;
                flags.novideo = true;
                flags.noaudio = true;
            }
            else if (arg == "--zoom" || arg.starts_with("--zoom="))
            {
                auto value = number(arg == "--zoom" ? next(i) : arg.substr(7));
                if (value < 1 || value > 16)
                    throw std::runtime_error("Zoom must be an integer from 1 to 16");
                flags.zoom = value;
            }
            else if (arg.starts_with("--speed="))
            {
                size_t used = 0;
                auto value = arg.substr(8);
                flags.speed = std::stof(value, &used);
                if (used != value.size() || !(flags.speed > 0) || !std::isfinite(flags.speed))
                    throw std::runtime_error("Invalid speed");
            }
            else if (arg.starts_with("--max-cycles="))
                limit = number(arg.substr(13));
            else if (arg.starts_with("--gpu="))
            {
                flags.gpu = arg.substr(6);
                if (flags.gpu != "default")
                    throw std::runtime_error("Only the default GPU is supported");
            }
            else if (arg == "-i" || arg == "--insert" || arg == "--insert-card")
            {
                auto address = number(next(i));
                auto file = next(i);
                if (address > 0xffff)
                    throw std::runtime_error("Device address exceeds 16 bits");
                (arg == "--insert-card" ? flags.cards : flags.inserts).emplace_back(address, file);
            }
            else
                throw std::runtime_error("Unknown option: " + arg);
        }
        if (flags.bootfile.empty())
        {
            help(argv[0]);
            return argc == 1 ? 0 : 1;
        }
        ANC216::Video::Window window;
        ANC216::EmemMapper mapper(flags, &window);
        ANC216::CPU cpu(&mapper, flags);
        window.set_input_handlers([&](uint16_t key)
                                  {
                                      mapper.keyboard_input(key);
                                  },
                                  [&](bool shutdown)
                                  {
                                      if (shutdown)
                                          cpu.request_shutdown();
                                      else
                                          cpu.request_soft_reset();
                                      if (flags.debug_mode)
                                          cpu.start();
                                  });
        std::signal(SIGINT, ANC216::handle_host_interrupt);
        if (flags.debug_mode)
            debug_console(cpu, mapper, window);
        else
        {
            // SDL stays on the main thread, but updates at display cadence rather
            // than once per emulated instruction.
            ANC216::ExecutionPacer pacer(flags);
            uint64_t cycles = 0;
            auto next_frame = std::chrono::steady_clock::now();
            while (!cpu.halted())
            {
                ANC216::poll_host_interrupt(cpu);
                const auto now = std::chrono::steady_clock::now();
                if (now >= next_frame)
                {
                    if (!window.poll())
                        break;
                    mapper.present();
                    next_frame = now + std::chrono::milliseconds(16);
                }
                for (unsigned i = 0; i < pacer.batch && !cpu.halted(); ++i)
                {
                    cpu.step();
                    if (limit && ++cycles >= limit && !cpu.halted())
                    {
                        std::cerr << "Instruction limit reached\n";
                        cpu.shutdown();
                        return 2;
                    }
                }
                if (!pacer.uncapped() && !cpu.halted())
                    std::this_thread::sleep_until(pacer.deadline());
            }
            mapper.present();
            cpu.shutdown();
        }
        if (!cpu.error().empty())
        {
            std::cerr << "emu: " << cpu.error() << '\n';
            return 1;
        }
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "emu: " << e.what() << '\n';
        return 1;
    }
}
