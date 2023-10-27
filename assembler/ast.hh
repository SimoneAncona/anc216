#include <string>
#include <vector>
#include "types.hh"

#pragma once

namespace ANC216
{
    enum RuleName
    {
        COMMAND,
        EXPRESSION,
        DECLARATION,
        INSTRUCTION_RULE,
        INSTRUCTION_LINE,
        ADDRESSING_MODE,
        EXPRESSION_LIST,
        SECTION,
        PREPROCESSOR,
        STRUCT_DEF,
        LABEL,
        BINARY_OP,
        UNARY_OP
    };

    class AST
    {
    private:
        bool final;
        Token token;
        RuleName rule_name;
        std::vector<AST> children;
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
            case BINARY_OP:
                return "binaryOperation";
            case UNARY_OP:
                return "unaryOperation";
            }
            return "";
        }

        std::string children_to_json()
        {
            std::string result = "[";
            for (size_t i = 0; i < children.size() - 1; i++)
                result += children[i].to_json() + ", ";
            if (children.size() > 0)
                result += children[children.size() - 1].to_json();
            return result + "]";
        }

        std::string token_type_to_string(TokenType type)
        {
            switch (type)
            {
            case IDENTIFIER:
                return "id";
            case NEW_LINE:
                return "newLine";
            case TYPE:
                return "type";
            case SEPARATOR:
                return "separator";
            case KEYWORD:
                return "keyword";
            case INSTRUCTION:
                return "instruction";
            }
            return "";
        }

    public:
        AST() = default;
        ~AST() = default;

        AST(RuleName rule_name)
        {
            this->rule_name = rule_name;
            final = false;
        }

        AST(const Token& token)
        {
            this->token = token;
            final = true;
        }

        inline bool is_final()
        {
            return final;
        }

        inline std::vector<AST> get_children()
        {
            return children;
        }

        inline RuleName get_rule_name()
        {
            return rule_name;
        }

        inline Token get_token()
        {
            return token;
        }

        inline AST *insert_non_terminal(RuleName rule_name)
        {
            children.push_back(AST(rule_name));
            return &children[children.size() - 1];
        }

        inline void insert_terminal(const Token &token)
        {
            children.push_back(AST(token));
        }

        std::string to_json()
        {
            if (!final)
                return "{\"rule\": \"" + rule_name_to_string(rule_name) + "\", \"children\": " + children_to_json() + "}";
            return "{\"tokenType\": \"" + token_type_to_string(token.type) + "\", \"value\": \"" + token.value + "\"}";
        }
    };
}