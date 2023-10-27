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
    ANC216::Parser parser("var x: byte");
    std::cout << parser.parse().to_json() << std::endl;
    return 0;
}