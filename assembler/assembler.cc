#include "../common/colors.hh"
#include <iostream>

int main(int argc, char **argv)
{
    std::cout << RED << "Ciao" << YELLOW << " come" << GREEN << " stai" << CYAN << "?" << std::endl
              << RESET;
    std::cout << RGB(0, 128, 128) << "Belin" << std::endl
              << RESET;
    return 0;
}