#include <iostream>

#include <emu.hh>
#include <console.hh>

enum Flag
{
    DEBUG,
    FAST_MODE,
    EXT,
    INSERT,
    INSERT_CARD,
    NOAUDIO,
    NOVIDEO,
    NOKEYBOARD,
    SPEED,
    HELP,
    GPU,
    BOOT,
    DEFAULT_CHARMAP,
    INSERT_CHARMAP
};

void print_help(char **);
void print_help_for_flag(Flag);

int main(int argc, char **argv)
{
    print_help(argv);
    return 0;
}

void print_help(char **argv)
{
    std::cout << "Usage:\n"
              << "\t" << argv[0] << YELLOW << " [options]\n"
              << RESET
              << "\nOptions:\n"

              << CYAN << "-b <file>" << RESET << "\t\t\t\t"
              << "select the binary file used for bootloader or OS"
              << "\n"
              << CYAN << "--boot <file>" << RESET << "\t\t\t\t"
              << "same as -b"
              << "\n"
              << CYAN << "-d" << RESET << "\t\t\t\t\t"
              << "run in debug mode"
              << "\n"
              << CYAN << "--debug" << RESET << "\t\t\t\t\t"
              << "same as -d"
              << "\n"
              << CYAN << "--default-charmap" << RESET << "\t\t\t"
              << "use the default charmap"
              << "\n"
              << CYAN << "--ext <address> <file>" << RESET << "\t\t\t"
              << "load the emulator with the specified extension"
              << "\n"
              << CYAN << "-f <file>" << RESET << "\t\t\t\t"
              << "run in fast mode"
              << "\n"
              << CYAN << "--fast-mode <file>" << RESET << "\t\t\t"
              << "same as -f"
              << "\n"
              << CYAN << "--gpu=<val>" << RESET << "\t\t\t\t"
              << "specify the emulated GPU to use"
              << "\n"
              << YELLOW << "     =default" << RESET << "\t\t\t\t"
              << "use the default API"
              << "\n"
              << YELLOW << "     =<file>" << RESET << "\t\t\t\t"
              << "use the specified GPU capable extension"
              << "\n"
              << CYAN << "--help [flag]" << RESET << "\t\t\t\t"
              << "show this help"
              << "\n"
              << CYAN << "-i <address> <file>" << RESET << "\t\t\t"
              << "specify a raw binary file that will be loaded in the emem using a virtual ROM"
              << "\n"
              << CYAN << "--insert <address> <file>" << RESET << "\t\t"
              << "same as -i"
              << "\n"
              << CYAN << "--insert-card <address> <file>" << RESET << "\t\t"
              << "insert a MPME card into a virtual slot"
              << "\n"
              << CYAN << "--insert-charmap <file>" << RESET << "\t\t\t"
              << "load the charmap into the GPU"
              << "\n"
              << CYAN << "--noaudio" << RESET << "\t\t\t\t"
              << "disable audio"
              << "\n"
              << CYAN << "--nokeyboard" << RESET << "\t\t\t\t"
              << "disable keyboard interrupts"
              << "\n"
              << CYAN << "--novideo" << RESET << "\t\t\t\t"
              << "disable video"
              << "\n"
              << CYAN << "--speed=<val>" << RESET << "\t\t\t\t"
              << "specify the emulation speed"
              << "\n"
              << YELLOW << "       =0.1"
              << "\n"
              << YELLOW << "       =0.5"
              << "\n"
              << YELLOW << "       =1"
              << "\n"
              << YELLOW << "       =2"
              << "\n"
              << YELLOW << "       =5" << RESET << "\n\n\n"
              << "For more information about a flag, digit --help [name of the flag]\n"
              << "for example --help --fast-mode";
}

void print_help_for_flag(Flag flag)
{
    switch (flag)
    {
    case BOOT:
        std::cout << "Usage:\n"
                  << CYAN << "\t--boot <file>" << RESET << "\n"
                  << "Aliases:\n"
                  << CYAN << "\t-b" << RESET << "\n"
                  << "The boot flag is used to specify a binary file that contains the software that will be executed first" << std::endl;
        return;
    case DEBUG:
        std::cout << "Usage:\n"
                  << CYAN << "\t--debug" << RESET << "\n"
                  << "Aliases:\n"
                  << CYAN << "\t-d" << RESET << "\n"
                  << "The emulator will start in debug mode. In this mode the user can stop the execution of the machine to see every little detail like RAM, registers and more.\nIn debug mode it is possible to debug the machine via commands given to the terminal.\nTo see the commands available in debug mode, run the emulator with this flag and then type help" << std::endl;
        return;
    case DEFAULT_CHARMAP:
        std::cout << "Usage:\n"
                  << CYAN << "\t--default-charmap" << RESET << "\n"
                  << "This flag can be set only if --gpu=default is set. With this flag set, the GPU will load the default charmap which consists of the set of textures for ascii characters" << std::endl;
        return;
    case EXT:
        std::cout << "Usage:\n"
                  << CYAN << "\t--ext <address> <file>" << RESET << "\n"
                  << "This flag allows you to load custom extensions for the emulator. The <address> specify where the device will be mapped in memory, the <file> specify the script of the device extension (.js or .py files)" << std::endl;
        return;
    }
}