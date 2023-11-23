#include <string>
#pragma once
#define RED "\u001b[31m"
#define GREEN "\u001b[32m"
#define YELLOW "\u001b[33m"
#define BLUE "\u001b[34m"
#define CYAN "\u001b[36m"
#define RESET "\u001b[0m"

#define OK(x) RESET << " Test " << x << " passed\n"
#define NO(x) RED << "X" << " Test " << x << " failed\n" << RESET
#define EXPECTED_BUT_GOT(x, y) "\t" << "Expected " << (std::string(x) == "\n" ? "new line" : x) << " but got " << (std::string(y) == "\n" ? "new line" : y) << "\n"

#define __PRINT_EXCEPTION(x) "\t" << RED << "Runtime exception: " << RESET << x.what() << "\n"

#define NO_WITH_ERR(x, e) NO(x) << __PRINT_EXCEPTION(e)