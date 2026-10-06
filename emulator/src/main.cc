#include <cpu.hh>
#include <emem.hh>
#include <video.hh>
#include <debug.hh>
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
                                      "  --speed=<positive number>     Instruction rate multiplier\n"
                                      "  -i, --insert <address> <file> Attach a raw ROM device\n"
                                      "  --insert-card <address> <file> Attach a writable MPME device\n"
                                      "  --insert-charmap <file>        Load AVC64 character/texture map\n"
                                      "  --gpu=default                 AVC64 (SDL build only)\n"
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
            else if (arg == "-f" || arg == "--fast-mode")
            {
                flags.fast_mode = true;
                flags.novideo = true;
                flags.noaudio = true;
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
        if (flags.debug_mode)
            debug_console(cpu, mapper, window);
        else
        {
            // The main thread owns SDL event/render calls. Headless runs use the same deterministic stepping path.
            uint64_t cycles = 0;
            while (!cpu.halted() && window.poll())
            {
                cpu.step();
                mapper.present();
                if (limit && ++cycles >= limit && !cpu.halted())
                {
                    std::cerr << "Instruction limit reached\n";
                    cpu.shutdown();
                    return 2;
                }
                if (!flags.fast_mode)
                    std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(10.0 / flags.speed));
            }
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
