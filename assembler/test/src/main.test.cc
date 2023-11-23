#include "assembler.test.hh"
#include "parser.test.hh"
#include "tokenizer.test.hh"
#include "common.hh"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

int main()
{
   
        while (fs::current_path().filename() != "test")
            fs::current_path("../");
        fs::current_path("cases");
        std::ifstream test1("test1.anc216");
        std::ifstream test2("test2.anc216");
        std::ifstream test3("test3.anc216");
        std::ifstream test4("test4.anc216");
        std::ifstream test5("test5.anc216");
        Test tests(test1, test2, test3, test4, test5);

        test_tokenizer(tests);
        return 0;
    
}