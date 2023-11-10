#include "ast.hh"
#include <vector>
#include <map>
#include "types.hh"
#include <tuple>
#include <string>
#include "isa.hh"

#pragma once

#define BYTE_S 8
#define WORD_S 16

namespace ANC216
{
    struct Label
    {
        size_t address;
        bool is_local;
        std::string module_name;
        std::string section;
    };

    struct Structure
    {
        std::map<std::string, char> fields;
    };

    struct Variable
    {
        size_t bp_relative_address;
        std::string label;
    };

    struct Operand
    {
        bool is_reg;
        std::string reg;
        size_t value;
        size_t size;
    };

    struct Instruction
    {
        AddressingMode addressing_mode;
        Operand op1;
        Operand op2;
        std::string instruction;
    };

    struct Environment
    {
        std::map<std::string, Label> labels;
        std::map<std::string, Structure> structures;
        std::map<std::string, Variable> variables;
        std::vector<Instruction> instructions;
    };

    class Analyzer
    {
    private:
        AST *ast;
        std::vector<Error> error_stack;
        Environment env;

        size_t current_address;
        std::string current_label;
        std::string current_section;

        void analyze_command(AST *ast)
        {
            for (auto el : ast->get_children())
            {
                switch (el->get_rule_name())
                {
                case SECTION:
                    analyze_section(el);
                    break;
                case STRUCT_DEF:
                    analyze_structure(el);
                    break;
                case DECLARATION:
                    analyze_declaration(el);
                    break;
                case LABEL:
                    analyze_label(el);
                    break;
                case INSTRUCTION_RULE:
                    analyze_instructions(el);
                    break;
                case EXPRESSION_LIST:
                    analyze_expression_list(el);
                    break;
                case COMMAND:
                    analyze_command(el);
                    break;
                case ORIGIN:
                    analyze_origin(el);
                    break;
                }
            }
        }

        void analyze_origin(AST *ast)
        {
            size_t value = eval_expression(ast->get_children()[1]);
            if (value < current_address)
            {
                error_stack.push_back({"Cannot set address, the address must be grater than or equal to the current address value", ast->get_children()[0]->get_token()});
                return;
            }
            current_address = value;
        }

        void analyze_section(AST *ast)
        {
            current_section = ast->get_children()[1]->get_token().value;
        }

        void analyze_structure(AST *ast)
        {
            env.structures[ast->get_children()[1]->get_token().value] = {};
            analyze_struct_el(ast->get_children()[3], ast->get_children()[1]->get_token().value);
        }

        void analyze_struct_el(AST *ast, const std::string &name)
        {
            env.structures[name].fields[ast->get_children()[0]->get_token().value] = ast->get_children()[2]->get_token().value == "byte" ? BYTE_S : WORD_S;
            if (ast->get_children()[ast->get_children().size() - 1]->get_rule_name() == STRUCT_DEF)
            {
                analyze_struct_el(ast->get_children()[ast->get_children().size() - 1], name);
            }
        }

        void analyze_declaration(AST *ast)
        {
        }

        void analyze_label(AST *ast)
        {
            bool is_local = false;
            size_t i = 0;
            if (*ast->get_children()[0] == "local")
            {
                is_local = true;
                i++;
            }
            else if (*ast->get_children()[0] == "global")
                i++;
            std::string name = ast->get_children()[i]->get_token().value;
            if (env.labels.find(name) != env.labels.end())
            {
                error_stack.push_back({"Redefinition of \"" + name + "\"", ast->get_children()[i]->get_token()});
                return;
            }
            env.labels[ast->get_children()[i]->get_token().value] = {current_address, is_local, ast->get_children()[i]->get_token().module_name};
            current_label = name;
        }

        void analyze_instructions(AST *ast)
        {
        }

        void analyze_expression_list(AST *ast)
        {
        }

        int eval_expression(AST *ast)
        {
            auto children = ast->get_children();
            if (ast->get_token() == "sizeof")
            {
                error_stack.push_back({"Cannot evaluate expression, cannot resolve the size of the label", ast->get_children()[0]->get_token()});
                return -1;
            }
            if (ast->get_token().type == IDENTIFIER)
            {
                error_stack.push_back({"Cannot evaluate expression, cannot resolve the address of \"" + ast->get_children()[0]->get_token().value + "\"", ast->get_children()[0]->get_token()});
                return -1;
            }
            if (ast->get_token().type == TYPE)
            {
                error_stack.push_back({"Unexpected type in expression", ast->get_children()[0]->get_token()});
                return -1;
            }
            if (children.size() == 0)
            {
                if (ast->get_token() == "$")
                    return static_cast<int>(current_address);

                if (ast->get_token().type == NUMBER_LITERAL)
                    return stoi(ast->get_token().value, nullptr, 0);
            }

            if (children.size() == 1)
            {
                if (ast->get_token() == "+")
                    return eval_expression(ast->get_children()[1]);

                if (ast->get_token() == "-")
                    return -eval_expression(ast->get_children()[1]);

                return eval_expression(children[0]);
            }

            int value = eval_expression(children[0]);
            if (children[1]->get_token() == "+")
            {
                return value + eval_expression(children[2]);
            }
            if (children[1]->get_token() == "-")
            {
                return value - eval_expression(children[2]);
            }
            if (children[1]->get_token() == "*")
            {
                return value * eval_expression(children[2]);
            }
            if (children[1]->get_token() == "/")
            {
                return value / eval_expression(children[2]);
            }
            return value;
        }

    public:
        Analyzer(AST *ast)
        {
            this->ast = ast;
            current_address = 0;
            current_label = "";
        }

        inline std::vector<char> assemble()
        {
            analyze_command(this->ast);
            if (!error_stack.empty())
                return {};

            return {};
        }

        inline std::vector<Error> &get_error_stack()
        {
            return error_stack;
        }
    };
}