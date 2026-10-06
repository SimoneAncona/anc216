#include <cpu.hh>
#include <debug.hh>
#include <emem.hh>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <video.hh>

void show_cpu_info(ANC216::CPUInfo info)
{
    for (unsigned i = 0; i < 8; ++i)
        std::cout << "R" << i << "=" << std::hex << std::setw(4)
                  << std::setfill('0') << uint16_t(info.reg[i])
                  << (i == 7 ? '\n' : ' ');
    std::cout << "PC=" << std::setw(4) << info.pc << " SP=" << std::setw(4)
              << info.sp << " BP=" << std::setw(4) << info.bp
              << " SR=" << std::setw(2) << unsigned(info.sr)
              << " instruction=" << std::setw(4) << info.current_instruction
              << std::dec << '\n';
}
void print_debug_help()
{
    std::cout << "ni / step: execute one instruction\nstart: run\nstop: "
                 "pause\nsh info: show registers\n"
              << "imem watch <address> <size>: inspect memory (decimal or 0x "
                 "hex)\nreset: hard reset\nexit: quit\n";
}
void debug_console(ANC216::CPU &cpu, ANC216::EmemMapper &mapper,
                   ANC216::Video::Window &window)
{
    cpu.stop();
    print_debug_help();
    std::string command;
    while (std::cout << "> " && std::getline(std::cin, command))
    {
        if (command == "exit")
            break;
        else if (command == "help")
            print_debug_help();
        else if (command == "ni" || command == "step")
        {
            cpu.step();
            show_cpu_info(cpu.get_info());
        }
        else if (command == "start")
            cpu.start();
        else if (command == "stop")
            cpu.stop();
        else if (command == "reset")
        {
            cpu.stop();
            cpu.load_init_state();
        }
        else if (command == "sh info")
            show_cpu_info(cpu.get_info());
        else if (command.starts_with("imem watch "))
        {
            try
            {
                std::istringstream stream(command.substr(11));
                std::string a, n;
                stream >> a >> n;
                auto address = std::stoul(a, nullptr, 0),
                     size = std::stoul(n, nullptr, 0);
                if (address >= MAX_MEM || size > MAX_MEM - address)
                    throw std::runtime_error("Out of range");
                for (unsigned i = 0; i < size; ++i)
                    std::cout << std::hex << std::setw(2) << std::setfill('0')
                              << unsigned(cpu.peek(address + i)) << ' ';
                std::cout << std::dec << '\n';
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
        }
        else
            std::cerr << "Unknown command\n";
        mapper.present();
        window.poll();
        if (cpu.halted())
        {
            std::cout << "CPU halted"
                      << (cpu.error().empty() ? "" : " : " + cpu.error()) << '\n';
        }
    }
    cpu.shutdown();
    cpu.wait();
}
