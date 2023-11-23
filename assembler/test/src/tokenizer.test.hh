#include "./console.hh"
#include <tokenizer.hh>
#include <types.hh>
#include <vector>
#include "common.hh"
#include <tuple>
#include <iostream>

#define TK_TEST_NAME(x, n) std::string("Tokenizer test ") + std::to_string(x) + ": " + n

#define TK_TEST1_EXPECTED                                  \
    {                                                      \
        "\n",                                              \
            "\n",                                          \
            "use", "PRINT_SYSCALL", "as", "0x05", "\n",    \
            "use", "EXIT_SYSCALL", "as", "0x00", "\n",     \
            "\n",                                          \
            "use", "STDOUT", "as", "0x00", "\n",           \
            "\n",                                          \
            "use", "EXIT_SUCCESS", "as", "0x00", "\n",     \
            "use", "EXIT_FAILURE", "as", "0x01", "\n",     \
            "\n",                                          \
            "section", ".", "data", "\n",                  \
            "message", ":", "\"Hello, world!\"", "\n",     \
            "message_len", ":", "$", "-", "message", "\n", \
            "\n",                                          \
            "section", ".", "text", "\n",                  \
            "global", "_code", ":", "\n",                  \
            "\n",                                          \
            "\n",                                          \
            "load", "l0", ",", "PRINT_SYSCALL", "\n",      \
            "load", "r1", ",", "message", "\n",            \
            "load", "r2", ",", "&", "message_len", "\n",   \
            "load",                                        \
            "r3", ",", "STDOUT", "\n",                     \
            "\n",                                          \
            "syscall",                                     \
            "\n",                                          \
            "\n",                                          \
            "\n",                                          \
            "cmp",                                         \
            "r7", ",", "0", "\n",                          \
            "jeq",                                         \
            "*", "no_error", "\n",                         \
            "if_error",                                    \
            ":", "\n",                                     \
            "load",                                        \
            "r1", ",", "EXIT_FAILURE", "\n",               \
            "jmp",                                         \
            "*", "exit", "\n",                             \
            "no_error",                                    \
            ":", "\n",                                     \
            "load",                                        \
            "r1", ",", "EXIT_SUCCESS", "\n",               \
            "exit",                                        \
            ":", "\n",                                     \
            "load",                                        \
            "r0", ",", "EXIT_SYSCALL", "\n",               \
            "syscall",                                     \
            "\n"                                           \
    }

using namespace ANC216;

void test_tokenizer1(std::filebuf *);
std::pair<std::string, std::string> assert_equal_tk(Tokenizer &, const std::vector<std::string>);
void print_tokens(Tokenizer &);

void test_tokenizer(Test &tests)
{
    test_tokenizer1(tests.test1.rdbuf());
}

void test_tokenizer1(std::filebuf *buf)
{
    const std::string test = TK_TEST_NAME(1, TEST_1);

    std::stringstream ss;
    ss << buf;
    AsmFlags flags;
    std::string str = ss.str();
    try
    {
        Tokenizer tokenizer(str, flags);
        auto result = assert_equal_tk(tokenizer, TK_TEST1_EXPECTED);
        if (result.first == "" && result.second == "")
        {
            std::cout << GREEN;
            std::wcout << "✓";
            std::cout << OK(test);
            return;
        }
        if (result.first == "|")
        {
            std::cerr << NO(test) << "\tDifferent sizes\n";
            print_tokens(tokenizer);
            return;
        }
        std::cerr << NO(test) << EXPECTED_BUT_GOT(result.first, result.second);
    }
    catch (const std::exception &e)
    {
        std::cerr << NO_WITH_ERR(test, e);
        return;
    }
}

std::pair<std::string, std::string> assert_equal_tk(Tokenizer &tk, const std::vector<std::string> tokens)
{
    // if (tk.get_tokens().size() - 1 != tokens.size())
    //     return {"|", "%"};
    for (size_t i = 0; i < tokens.size(); i++)
    {
        if (tk.get_tokens()[i] != tokens[i])
            return {tokens[i], tk.get_tokens()[i].value};
    }
    return {"", ""};
}

void print_tokens(Tokenizer &tk)
{
    auto tokens = tk.get_tokens();
    for (auto &token : tokens)
    {
        if (token.type == NEW_LINE)
        {
            std::cout << "\xaa\n";
            continue;
        }

        if (token.type == KEYWORD || token.type == INSTRUCTION)
            std::cout << CYAN;
        else if (token.type == NUMBER_LITERAL)
            std::cout << YELLOW;
        else if (token.type == STRING_LITERAL)
            std::cout << GREEN;

        std::cout << token.value << RESET << " ";
    }
}