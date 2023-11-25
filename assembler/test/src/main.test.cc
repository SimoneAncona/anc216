#include "assembler.test.hh"
#include "parser.test.hh"
#include "tokenizer.test.hh"
#include "common.hh"
#include <fstream>
#include <filesystem>
#include <sstream>

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
        std::stringstream stest1, stest2, stest3, stest4, stest5;
        stest1 << test1.rdbuf();
        stest2 << test2.rdbuf();
        stest3 << test3.rdbuf();
        stest4 << test4.rdbuf();
        stest5 << test5.rdbuf();
        Test tests(stest1.str(), stest2.str(), stest3.str(), stest4.str(), stest5.str());
        test1.close();
        test2.close();
        test3.close();
        test4.close();
        test5.close();

        test_tokenizer(tests);
        parser_test(tests);
        assembler_test(tests);
        return 0;
    
}