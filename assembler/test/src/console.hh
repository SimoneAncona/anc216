#include <string>
#pragma once
#define RED "\u001b[31m"
#define GREEN "\u001b[32m"
#define YELLOW "\u001b[33m"
#define BLUE "\u001b[34m"
#define CYAN "\u001b[36m"
#define RESET "\u001b[0m"

#define OK(x) GREEN << "✓" << RESET << " Test " << x << " passed\n"
#define NO(x) RED << "✗" << " Test " << x << " failed\n"
#define EXPECTED_BUT_GOT(x, y) "\t" << "Expected " << x << " but got " << y << "\n"