#include <console.hh>
#include <parser.hh>
#include <analyzer.hh>
#include <types.hh>
#include <assembler.hh>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>

#define ASSEMBLER_VERSION_MAJOR 1
#define ASSEMBLER_VERSION_MINOR 0
#define ASSEMBLER_VERSION_PATCH 0

#define ASSEMBLY_STANDARD_VERSION "ANC216.1"
#define ASSEMBLY_PREVIEW_STANDARD_VERSION "ANC216.2"

namespace fs = std::filesystem;

void print_error_stack(std::vector<ANC216::Error>);
void print_help(char **);
void print_version();
ANC216::AsmFlags get_flags(int, char **);

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
    ANC216::AsmFlags flags = get_flags(argc, argv);
    fs::path p(fs::weakly_canonical(flags.input_file));
    flags.input_file = p.string();
    fs::current_path(p.parent_path());
    std::ifstream file = std::ifstream(flags.input_file);
    std::stringstream ss;
    ss << file.rdbuf();
    std::string file_string = ss.str();

    ANC216::Parser parser(file_string, flags.input_file, flags);
    ANC216::AST *res = parser.parse();
    if (parser.get_error_stack().size() != 0)
    {
        print_error_stack(parser.get_error_stack());
    }
    if (parser.has_errors())
        return -1;

    ANC216::Analyzer analyzer(res);
    analyzer.analyze();

    if (analyzer.get_error_stack().size() != 0)
    {
        print_error_stack(analyzer.get_error_stack());
    }
    if (analyzer.has_errors())
        return -1;

    ANC216::Assembler assembler(analyzer.get_environment());
    auto a = assembler.assemble();
    if (assembler.get_error_stack().size() != 0)
    {
        print_error_stack(assembler.get_error_stack());
    }
    if (assembler.has_errors())
        return -1;

    return 0;
}

ANC216::AsmFlags get_flags(int argc, char **argv)
{
    ANC216::AsmFlags flags;
    std::string arg;
    bool skip = false;
    for (int i = 1; i < argc; i++)
    {
        arg = std::string(argv[i]);
        if (skip)
        {
            if (flags.input_file == "")
            {
                flags.input_file = arg;
                skip = false;
                continue;
            }
            if (flags.output_file == "")
            {
                flags.output_file = arg;
                skip = false;
                continue;
            }
            skip = false;
        }
        if (arg == "--")
        {
            skip = true;
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
        if (arg == "-ud" || arg == "--use-debug")
        {
            flags.use_as.push_back({"_DEBUG", ""});
            continue;
        }
        if (arg == "--preview")
        {
            std::cout << YELLOW << "Warning:\n\t" << RESET << "You are using the preview assembly standard, this standard is " << RED << "NOT" << RESET << " fully implemented\n"
                      << std::endl;
            flags.preview = true;
            continue;
        }
        if (arg == "-t")
        {
            flags.get_time = true;
            continue;
        }
        if (arg == "-S")
        {
            flags.output_size = true;
            continue;
        }
        if (arg == "-s")
        {
            flags.get_symbol_table = true;
            continue;
        }

        if (arg == "-i")
        {
            int j;
            for (j = i + 1; j < argc; j++)
            {
                arg = std::string(argv[j]);
                if (arg == "--")
                {
                    skip = true;
                    continue;
                }
                if (skip || !arg.starts_with("-"))
                {
                    if (!fs::exists(arg))
                    {
                        std::cerr << RED << "Path not found: " << RESET << "cannot find '" + flags.input_file + "'\n"
                                  << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    flags.import_paths.push_back(fs::weakly_canonical(arg).string());
                }
                break;
            }
            i = j - 1;
            skip = false;
            continue;
        }

        if (arg == "-u")
        {
            if (i == argc - 1)
            {
                std::cerr << RED << "Unexpected end: " << RESET << "after '-u'\n"
                          << std::endl;
                exit(EXIT_FAILURE);
            }
            arg = std::string(argv[i + 1]);
            if (!iswalpha(arg[0]) && arg[0] != '_')
            {
                std::cerr << RED << "Invalid argument: " << RESET << "expected an identifier after '-u'\n"
                          << std::endl;
                exit(EXIT_FAILURE);
            }
            flags.use_as.push_back({arg, ""});
            i++;
            if (i != argc - 1 && std::string(argv[i + 1]) == "-a")
            {
                arg = std::string(argv[i + 1]);
                i++;
                if (i == argc - 1)
                {
                    std::cerr << RED << "Unexpected end: " << RESET << "after '-a'\n"
                              << std::endl;
                    exit(EXIT_FAILURE);
                }
                flags.use_as[flags.use_as.size() - 1].second = std::string(argv[i + 1]);
                i++;
            }
            continue;
        }

        if (flags.input_file == "")
        {
            flags.input_file = arg;
            continue;
        }
        if (flags.output_file == "")
        {
            flags.output_file = arg;
            continue;
        }
    }
    if (flags.input_file == "")
    {
        std::cerr << RED << "Source file expected: " << RESET << "no source file was given\n"
                  << std::endl;
        exit(EXIT_FAILURE);
    }
    if (!fs::exists(flags.input_file))
    {
        std::cerr << RED << "File not found: " << RESET << "cannot find '" + flags.input_file + "'\n"
                  << std::endl;
        exit(EXIT_FAILURE);
    }
    if (flags.output_file == "")
        flags.output_file = "a.out";
    return flags;
}

void print_error_stack(std::vector<ANC216::Error> error_stack)
{
    size_t i = 1;
    for (auto &error : error_stack)
    {
        std::cout << (error.is_warning() ? YELLOW : RED) << "[" << i << "] " << error.to_string() << std::endl;
        i++;
    }
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
              << CYAN << "-i <path> ..." << RESET << "\t\t"
              << "Import a library or set import path"
              << "\n"
              << CYAN << "--preview" << RESET << "\t\t"
              << "Use the new standard preview"
              << "\n"
              << CYAN << "-S" << RESET << "\t\t\t"
              << "Print the size of the output file"
              << "\n"
              << CYAN << "-s" << RESET << "\t\t\t"
              << "Generate symbol table constants"
              << "\n"
              << CYAN << "--stdlib <dir>" << RESET << "\t\t"
              << "Set standard library path"
              << "\n"
              << CYAN << "-t" << RESET << "\t\t\t"
              << "Print completation time"
              << "\n"
              << CYAN << "-u <const> [-a <value>]" << RESET << "\t"
              << "Use as"
              << "\n"
              << CYAN << "-ud" << RESET << "\t\t\t"
              << "Use _DEBUG"
              << "\n"
              << CYAN << "--use-debug" << RESET << "\t\t"
              << "Use _DEBUG"
              << "\n"
              << CYAN << "-v" << RESET << "\t\t\t"
              << "Print version information"
              << "\n"
              << CYAN << "--version" << RESET << "\t\t"
              << "Print version information"
              << "\n"
              << CYAN << "-w" << RESET << "\t\t\t"
              << "Suppress all warnings"
              << "\n"
              << "\n"
              << "<mandatory>\t" << YELLOW << "[optional]\n"
              << RESET;
}

void print_version()
{
    std::cout << "Assembler version:"
              << "\t\t" << GREEN << ASSEMBLER_VERSION_MAJOR << "." << ASSEMBLER_VERSION_MINOR << "." << ASSEMBLER_VERSION_PATCH << "\n"
              << RESET << "Assembly standard:"
              << "\t\t" << GREEN << ASSEMBLY_STANDARD_VERSION << "\n"
              << RESET << "Assembly standard preview:"
              << "\t" << GREEN << ASSEMBLY_PREVIEW_STANDARD_VERSION << "\n"
              << RESET;
}