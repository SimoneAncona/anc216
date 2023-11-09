#include "ast.hh"
#include <vector>
#include <map>
#include "types.hh"

#pragma once

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

    class Analyzer
    {
    private:
        AST *ast;
        std::vector<Error> error_stack;
        std::map<std::string, Label> labels;
        std::map<std::string, Structure> structures;
        std::map<std::string, Variable> variables;

        size_t current_address;
        std::string current_label;
        std::string current_section;

        void analyze_command(AST *ast)
        {
            for (auto &ast : ast->get_children())
            {
                switch (ast->get_rule_name())
                {
                case SECTION:
                    analyze_section(ast);
                    return;
                case STRUCT_DEF:
                    analyze_structure(ast);
                    return;
                case DECLARATION:
                    analyze_declaration(ast);
                    return;
                case LABEL:
                    analyze_label(ast);
                    return;
                case INSTRUCTION_RULE:
                    analyze_instructions(ast);
                    return;
                case EXPRESSION_LIST:
                    analyze_expression_list(ast);
                    return;
                }
            }
        }

        void analyze_section(AST *ast)
        {
            current_section = ast->get_children()[1]->get_token().value;
            analyze_command(ast->get_children()[2]);
        }

        void analyze_structure(AST *ast)
        {
            Structure strct;
            analyze_struct_el(ast, strct);
        }

        void analyze_struct_el(AST *ast, Structure strct)
        {
            
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
            else if (*ast->get_children()[0] == "global") i++;
            std::string name = ast->get_children()[i]->get_token().value;
            if (labels.find(name) != labels.end())
            {
                error_stack.push_back({"Redefinition of \"" + name + "\"", ast->get_children()[i]->get_token()});
                return;
            }
            labels[ast->get_children()[i]->get_token().value] = {current_address, is_local, ast->get_children()[i]->get_token().module_name};
            analyze_command(ast->get_children()[i + 1]);
        }

        void analyze_instructions(AST *ast)
        {

        }

        void analyze_expression_list(AST *ast)
        {

        }

        void analyze_expression(AST *ast)
        {

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