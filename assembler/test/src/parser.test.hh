#include "./console.hh"
#include <parser.hh>
#include <ast.hh>
#include <types.hh>
#include <vector>
#include "common.hh"
#include <tuple>
#include <iostream>

using namespace ANC216;

#define PR_TEST_NAME(x, n) std::string("Parser test ") + std::to_string(x) + ": " + n

#define PR_TEST1_EXPECTED                                                                        \
    std::vector<RuleName>{                                                                                            \
        SECTION, LABEL, EXPRESSION_LIST, LABEL, EXPRESSION_LIST,                                      \
            SECTION, LABEL, INSTRUCTION_RULE, INSTRUCTION_RULE, INSTRUCTION_RULE,                \
            INSTRUCTION_RULE, INSTRUCTION_RULE, INSTRUCTION_RULE, INSTRUCTION_RULE, LABEL, INSTRUCTION_RULE,       \
            INSTRUCTION_RULE, LABEL, INSTRUCTION_RULE, LABEL, INSTRUCTION_RULE, INSTRUCTION_RULE \
    }

void parser_test1(std::string &);
std::pair<RuleName, RuleName> assert_equal_pr(AST *, std::vector<RuleName>, size_t = 0);
std::string rule_name_to_string(RuleName);

void parser_test(Test &tests)
{
    parser_test1(tests.test1);
}

void parser_test1(std::string &str)
{
    const std::string test_name = PR_TEST_NAME(1, TEST_1);

    AsmFlags flags;
    try
    {
        Parser parser(str, "test1", flags);
        auto result = assert_equal_pr(parser.parse(), PR_TEST1_EXPECTED);
        if (result.first == NONE && result.second == NONE)
        {
            std::cout << OK(test_name);
            return;
        }
        std::cerr << NO(test_name) << EXPECTED_BUT_GOT(rule_name_to_string(result.first), rule_name_to_string(result.second));
    }
    catch (const std::exception &e)
    {
        std::cerr << NO_WITH_ERR(test_name, e);
        return;
    }
}
std::string rule_name_to_string(RuleName rule_name)
{
    switch (rule_name)
    {
    case COMMAND:
        return "command";
    case EXPRESSION:
        return "expression";
    case DECLARATION:
        return "declaration";
    case INSTRUCTION_RULE:
        return "instruction";
    case INSTRUCTION_LINE:
        return "instruction";
    case ADDRESSING_MODE:
        return "addressingMode";
    case ADDRESSING_MODE_IMMEDIATE:
        return "addressingModeImmediate";
    case ADDRESSING_MODE_MEMORY_TO_MEMORY:
        return "memoryToMemory";
    case ADDRESSING_MODE_MEMORY_TO_REGISTER:
        return "memoryToRegister";
    case ADDRESSING_MODE_REGISTER_TO_REGISTER:
        return "registerToRegister";
    case ADDRESSING_MODE_REGISTER:
        return "registerAccess";
    case ADDRESSING_MODE_MEMORY:
        return "memoryAccess";
    case ADDRESSING_MODE_INDIRECT:
        return "indirectAccess";
    case ADDRESSING_MODE_ABSOLUTE:
        return "absoluteAddressingMode";
    case ADDRESSING_MODE_REALTIVE_BP:
        return "relativeToBP";
    case ADDRESSING_MODE_REALTIVE_PC:
        return "relative";
    case ADDRESSING_MODE_RELATIVE_ARRAY:
        return "arrayAccess";
    case EXPRESSION_LIST:
        return "expressionList";
    case SECTION:
        return "section";
    case PREPROCESSOR:
        return "preProcessor";
    case STRUCT_DEF:
        return "structDefinition";
    case LABEL:
        return "label";
    case ORIGIN:
        return "origin";
    case BINARY_OP:
        return "binaryOperation";
    case UNARY_OP:
        return "unaryOperation";
    }
    return "none";
}

std::pair<RuleName, RuleName> assert_equal_pr(AST *ast, std::vector<RuleName> rules, size_t next_rule)
{
    if (ast->get_children().size() == 0 && next_rule >= rules.size())
        return {NONE, NONE};
    if (ast->get_children().size() != 0 && next_rule >= rules.size())
    {
        return {NONE, ast->get_rule_name()};
    }
    if (ast == nullptr && next_rule < rules.size())
    {
        return {rules[next_rule], NONE};
    }
    if (ast->get_children().size() == 0 && next_rule < rules.size())
    {
        return {rules[next_rule], NONE};
    }
    if (ast->get_children()[0]->get_rule_name() != rules[next_rule])
    {
        return {rules[next_rule], ast->get_children()[0]->get_rule_name()};
    }
    if (ast->get_children().size() == 1 && next_rule < rules.size())
    {
        return {rules[next_rule], NONE};
    }
   
    return assert_equal_pr(ast->get_children()[ast->get_children().size() - 1], rules, next_rule + 1);
}