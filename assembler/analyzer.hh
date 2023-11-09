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

        void analyze_command(AST *ast)
        {
            for (auto &ast : ast->get_children())
            {
                switch (ast->get_rule_name())
                {
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

        void analyze_structure(AST *ast)
        {

        }

        void analyze_declaration(AST *ast)
        {

        }

        void analyze_label(AST *ast)
        {
            
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

        std::vector<char> assemble()
        {
            analyze_command(this->ast);
            if (!error_stack.empty())
                return;
        }
    };
}