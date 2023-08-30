#include <string>
#define RED "\u001b[31m"
#define GREEN "\u001b[32m"
#define YELLOW "\u001b[33m"
#define BLUE "\u001b[34m"
#define CYAN "\u001b[36m"
#define RESET "\u001b[0m"
#define RGB(R, G, B) std::string("\u001b[38;2;") + std::to_string(R) + std::string(";") + std::to_string(G) + std::string(";") + std::to_string(B) + std::string("m")