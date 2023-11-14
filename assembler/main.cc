#include "console.hh"
#include "parse.hh"
#include "analyzer.hh"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include "types.hh"

#define ASSEMBLER_VERSION_MAJOR 1
#define ASSEMBLER_VERSION_MINOR 0
#define ASSEMBLER_VERSION_PATCH 0

#define ASSEMBLY_STANDARD_VERSION "ANC216.1"

namespace fs = std::filesystem;

void print_error_stack(std::vector<ANC216::Error>);

int main(int argc, char **argv)
{
    std::string filename = "./test.anc216";
    fs::path p(fs::weakly_canonical(filename));
    filename = p.string();
    fs::current_path(p.parent_path());
    std::ifstream file = std::ifstream(filename);
    std::stringstream ss;
    ss << file.rdbuf();
    std::string file_string = ss.str();
    if (argc < 2)
    {
        std::cout << "Usage:\n"
                  << "\t" << argv[0] << " <source file> " << YELLOW << "[output file]\n"
                  << std::endl
                  << RESET;
        return 0;
    }
    ANC216::Parser parser(file_string, filename);
    ANC216::AST *res = parser.parse();
    if (parser.get_error_stack().size() != 0)
    {
        print_error_stack(parser.get_error_stack());
        return -1;
    }

    ANC216::Analyzer analyzer(res);
    analyzer.assemble();

    if (analyzer.get_error_stack().size() != 0)
    {
        print_error_stack(analyzer.get_error_stack());
        return -1;
    }
    return 0;
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