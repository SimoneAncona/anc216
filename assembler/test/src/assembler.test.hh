#include "./console.hh"
#include <assembler.hh>
#include <parser.hh>
#include <analyzer.hh>
#include "common.hh"
#include <tuple>

using namespace ANC216;

#define AS_TEST_NAME(x, n) std::string("Assembler test ") + std::to_string(x) + ": " + n

#define AS_TEST1_EXPECTED                           \
    std::vector<unsigned char>                      \
    {                                               \
        'H', 'e', 'l', 'l', 'o', ',', ' ',          \
            'W', 'o', 'r', 'l', 'd', '!', 0x00, 13, \
            0b11000101, 0x3a, 0x05,                 \
            0b11001001, 0x3a, 0x00, 0x00,           \
            0b11010000, 0x3a, 0x00, 13,             \
            0b11011001, 0x3a, 0x00, 0x00,           \
            0b00000000, 0x03,                       \
            0b11111001, 0x20, 0x00, 0x00,           \
            0b10000100, 0x23, 7,                    \
            0b11001001, 0x3a, 0x00, 0x01,           \
            0b10000100, 0x22, 4,                    \
            0b11001001, 0x3a, 0x00, 0x00,           \
            0b11000101, 0x3a, 0x00,                 \
            0b00000000, 0x03                        \
    }

void assembler_test1(std::string &str);
std::pair<unsigned char, unsigned char> assert_equal_as(Assembler &, std::vector<unsigned char>);

void assembler_test(Test &tests)
{
    assembler_test1(tests.test1);
}

std::string to_hex_string(unsigned char val)
{
    std::string str;
    if (val == 0)
        return "0x00";
    while (val > 0)
    {
        str = std::string(1, ((val % 16 < 10) ? val % 16 + '0' : (val % 16) - 10 + 'a')) + str;
        val /= 16;
    }
    if (str.size() == 1)
        str = "0" + str;
    return "0x" + str;
}

void assembler_test1(std::string &str)
{
    const std::string test_name = AS_TEST_NAME(1, TEST_1);

    AsmFlags flags;
    try
    {
        Parser parser(str, "test1", flags);
        Analyzer analyzer(parser.parse());
        analyzer.analyze();
        Assembler assembler(analyzer.get_environment());
        auto res = assert_equal_as(assembler, AS_TEST1_EXPECTED);
        if (res.first == 0 && res.second == 0)
        {
            std::cout << OK(test_name);
            return;
        }
        if (res.first == 255 && res.second == 255)
        {
            std::cerr << NO(test_name) << "\tDifferent sizes";
            return;
        }
        std::cerr << NO(test_name) << EXPECTED_BUT_GOT(to_hex_string(res.first), to_hex_string(res.second));
    }
    catch (const std::exception &e)
    {
        std::cerr << NO_WITH_ERR(test_name, e);
        return;
    }
}

std::pair<unsigned char, unsigned char> assert_equal_as(Assembler &as, std::vector<unsigned char> bin)
{
    auto assemble = as.assemble();
    if (assemble.size() != bin.size())
        return {255, 255};
    for (auto i = 0; i < bin.size(); i++)
        if (bin[i] != assemble[i])
            return {bin[i], assemble[i]};
    return {0, 0};
}