#include <iostream>

#include <console.hh>

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

void print_help(char **argv);
void print_version(void);

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Usage:\n"
                  << "\t" << argv[0] << " <source file> " << YELLOW << "[output file]\n"
                  << std::endl
                  << RESET;
        return 0;
    }
    if (std::string(argv[1]) == "--help")
    {
        print_help(argv);
        return 0;
    }
    if (std::string(argv[1]) == "--version" || std::string(argv[1]) == "-v")
    {
        print_version();
        return 0;
    }
    return 0;
}

void print_help(char **argv)
{

}

void print_version()
{
    std::cout << "Asembler version:" << "\t\t" << GREEN << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH << std::endl;
}