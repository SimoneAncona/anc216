#include "../common/colors.hh"
#include "parse.hh"
#include <iostream>
#include <string>

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
    ANC216::Parser parser(
        R"(
        _code:
            read [0x345 * 367 + 5]
        )");
    std::cout << parser.parse().to_json() << std::endl;

    while (parser.get_error_stack().size() != 0)
    {
        std::cout << parser.get_error_stack().top().to_string() << std::endl;
        parser.get_error_stack().pop();
    }
    return 0;
}