#include "../common/colors.hh"
#include "parse.hh"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

int main(int argc, char **argv)
{
    std::ifstream file = std::ifstream("test.anc216");
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
    ANC216::Parser parser(file_string);
    ANC216::AST *res = parser.parse();
    if (parser.get_error_stack().size() == 0)
        std::cout << res->to_json() << std::endl;

    while (parser.get_error_stack().size() != 0)
    {
        std::cout << parser.get_error_stack().top().to_string() << std::endl;
        parser.get_error_stack().pop();
    }
    return 0;
}