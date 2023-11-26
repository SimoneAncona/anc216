#include <iostream>
#include <filesystem>
#include <fstream>

#include <console.hh>
#include <disassembler.hh>

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0

void print_help(char **argv);
void print_version(void);

namespace fs = std::filesystem;

struct Flags
{
    std::string header;
    unsigned char print_stdout : 1;
};

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Usage:\n"
                  << "\t" << argv[0] << " <source file> " << YELLOW << "[output file]\n"
                  << "\n"
                  << RESET
                  << "Type '--help' for more information"
                  << "\n"
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
    std::string in_filename;
    std::string out_filename;
    Flags flags;
    std::string arg;
    for (size_t i = 1; i < argc; i++)
    {
        arg = std::string(argv[i]); 
        if (arg.starts_with("-"))
        {
            if (arg == "--stdout")
            {
                flags.print_stdout = true;
                continue;
            }
            if (arg.starts_with("-h="))
            {
                if (arg != "-h=ualf")
                {
                    std::cerr << RED << "Unrecognized header: " << RESET << "options are:"
                              << "\n\t" << YELLOW << "ualf" << RESET << std::endl;
                    exit(EXIT_FAILURE);
                }
                flags.header = arg.substr(3);
                continue;
            }
            if (arg != "--")
            {
                std::cerr << RED << "Urecognized flag: " << RESET << arg;
                exit(EXIT_FAILURE);
            }
            if (i != argc)
            {
                i++;
                arg = argv[i];
            }
        }
        if (in_filename == "")
        {
            in_filename = arg;
            continue;
        }
        if (out_filename == "")
        {
            out_filename = arg;
            continue;
        }
        std::cerr << RED << "Unexpected argument: " << RESET << arg;
        exit(EXIT_FAILURE);
    }
    if (!fs::exists(in_filename))
    {
        std::cerr << RED << "File not found: " << RESET << "cannot find '" + in_filename + '"' << std::endl;
        exit(EXIT_FAILURE);
    }

    fs::path p(fs::weakly_canonical(in_filename));
    in_filename = p.string();
    fs::current_path(p.parent_path());
    if (out_filename.empty())
        out_filename = "a.anc216";

    std::ifstream in(in_filename, std::ios::binary);
    ANC216::Disassembler dis(in, flags.header);
    std::ofstream out(out_filename, std::ios::binary);
    out << dis.disassemble();
    out.close();
    return 0;
}

void print_help(char **argv)
{
    std::cout << "Usage:\n"
              << "\t" << argv[0] << " <source file> " << YELLOW << "[output file]\n"
              << RESET
              << "\nOptions:\n"
              << CYAN << "-h=<header>" << RESET << "\t\t"
              << "Set a file header"
              << "\n"
              << YELLOW << "  =ualf" << RESET << "\t\t\t"
              << "Set UALf header"
              << "\n"
              << CYAN << "--help" << RESET << "\t\t\t"
              << "Print this message"
              << "\n"
              << CYAN << "--stdout" << RESET << "\t\t"
              << "Print the output in the stdout"
              << "\n"
              << CYAN << "-v" << RESET << "\t\t\t"
              << "Print version information"
              << "\n"
              << CYAN << "--version" << RESET << "\t\t"
              << "Print version information"
              << "\n"
              << "\n"
              << "<mandatory>\t" << YELLOW << "[optional]\n"
              << RESET;
}

void print_version()
{
    std::cout << "Disassembler version:"
              << "\t" << GREEN << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH
              << RESET << std::endl;
}