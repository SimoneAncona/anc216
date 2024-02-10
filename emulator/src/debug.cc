#include <debug.hh>
#include <thread>

void show_cpu_info(ANC216::CPUInfo info)
{
    #ifdef __WIN32__ 
        system("cls");
    #else
        system("clear");
    #endif
    std::cout << "R0 " << std::hex << info.reg[0] << std::endl; 
    std::cout << "R1 " << std::hex << info.reg[1] << std::endl; 
    std::cout << "R2 " << std::hex << info.reg[2] << std::endl; 
    std::cout << "R3 " << std::hex << info.reg[3] << std::endl; 
    std::cout << "R4 " << std::hex << info.reg[4] << std::endl; 
    std::cout << "R5 " << std::hex << info.reg[5] << std::endl; 
    std::cout << "R6 " << std::hex << info.reg[6] << std::endl; 
    std::cout << "R7 " << std::hex << info.reg[7] << std::endl;
    std::cout << "PC " << std::hex << info.pc << std::endl;
    std::cout << "SP " << std::hex << info.sp << std::endl;
    std::cout << "BP " << std::hex << info.bp << std::endl;
    std::cout << "SR " << std::hex << info.sr << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void debug_console(ANC216::CPU &emu, ANC216::EmemMapper &mapper, ANC216::Video::Window &window)
{
    emu.stop();
    std::cout << "The emulator is currently stopped, type 'start' to start.\nType 'help' for more information about the debug console." << std::endl;
    std::string command;
    bool is_running = false;
    while (true)
    {
        std::cout << CYAN << "> " << RESET;
        std::getline(std::cin, command);
        if (command == "help")
            print_debug_help();
        else if (command == "start")
        {
            emu.start();
            is_running = true;
        }
        else if (command == "stop")
        {
            emu.stop();
            is_running = false;
        }
        else if (command == "exit")
            exit(EXIT_SUCCESS);
        else if (command == "sh info")
        {
            if (is_running)
                do
                {
                    show_cpu_info(emu.get_info());
                    std::cout << "Press enter to exit" << std::endl;
                    std::cout << (int)window.key_pressed() << std::endl;
                    
                } while (window.key_pressed() != 0x0d);
            else
            {
                std::cout << "R0 " << std::hex << emu.get_registers()[0] << std::endl; 
                std::cout << "R1 " << std::hex << emu.get_registers()[1] << std::endl; 
                std::cout << "R2 " << std::hex << emu.get_registers()[2] << std::endl; 
                std::cout << "R3 " << std::hex << emu.get_registers()[3] << std::endl; 
                std::cout << "R4 " << std::hex << emu.get_registers()[4] << std::endl; 
                std::cout << "R5 " << std::hex << emu.get_registers()[5] << std::endl; 
                std::cout << "R6 " << std::hex << emu.get_registers()[6] << std::endl; 
                std::cout << "R7 " << std::hex << emu.get_registers()[7] << std::endl; 

                std::cout << "PC " << std::hex << emu.get_pc() << std::endl;
            }
        }
        else if (command == "ni")
            emu.step();
        else
            PRINT_DBG_ERROR("Unknown command");
    }
}

void print_debug_help()
{
    std::cout   << "****************************************************************\n"
                << "*                         DEBUG COMMANDS                       *\n"
                << "****************************************************************\n"
                << "\n\n"
                << GREEN << "Debugging:\n\n" << RESET

                << "\t" << CYAN << "b " << YELLOW << "<address>" << RESET << "\t\t\t\tSet a breakpoint\n"
                << "\t" << CYAN << "rm " << YELLOW << "<breakpoint id>" << RESET << "\t\t\tRemove a breakpoint\n"
                << "\t" << CYAN << "rm all" << RESET << "\t\t\t\t\tRemove all breakpoints\n"
                << "\t" << CYAN << "ls b" << RESET << "\t\t\t\t\tLists all breakpoints\n"
                
                << GREEN << "Diagnostics:\n\n" << RESET

                << "\t" << CYAN << "emem watch " << YELLOW << "<address> <size>" << RESET << "\t\tShow the content of the external memory\n"
                << "\t" << CYAN << "imem watch " << YELLOW << "<address> <size>" << RESET << "\t\tShow the content of the internal memory\n"
                << "\t" << CYAN << "sh info" << RESET << "\t\t\t\t\tShow general information about the CPU\n"
                << "\t" << CYAN << "stk watch " << YELLOW << "<size>" << RESET << "\t\t\tShow the content of the stack memory\n"
                << "\t" << CYAN << "vmem watch " << YELLOW << "<address> <size>" << RESET << "\t\tShow the content of the video memory\n"

                << GREEN << "Execution:\n\n" << RESET

                << "\t" << CYAN << "ni" << RESET << "\t\t\t\t\tExecute next instrucion\n"
                << "\t" << CYAN << "nj" << RESET << "\t\t\t\t\tExecute until next jump instruction\n"
                << "\t" << CYAN << "nr" << RESET << "\t\t\t\t\tExecute until next return instruction\n"
                << "\t" << CYAN << "reset" << RESET << "\t\t\t\t\tHard reset\n"
                << "\t" << CYAN << "stop" << RESET << "\t\t\t\t\tStop the execution\n"
                << "\t" << CYAN << "start" << RESET << "\t\t\t\t\tStart the execution\n"

                << GREEN << "I/O:\n\n" << RESET

                << "\t" << CYAN << "dis audio" << RESET << "\t\t\t\tDisable audio\n"
                << "\t" << CYAN << "dis video" << RESET << "\t\t\t\tDisable video\n"
                << "\t" << CYAN << "en audio" << RESET << "\t\t\t\tEnable audio\n"
                << "\t" << CYAN << "en video" << RESET << "\t\t\t\tEnable video\n"
                << "\t" << CYAN << "ins card" << YELLOW << "<file>" << RESET << "\t\t\t\tInsert a card\n"
                << "\t" << CYAN << "ins char" << YELLOW << "<file>" << RESET << "\t\t\t\tInsert charmap\n"

                << GREEN << "Interrupts:\n\n" << RESET
                << GREEN << "Memory\n\n" << RESET
                << GREEN << "Other:\n\n" << RESET

                << "\t" << CYAN << "exit" << RESET << "\t\tExit the emulation\n"
                << "\t" << CYAN << "help" << RESET << "\t\tShow this list\n";

}