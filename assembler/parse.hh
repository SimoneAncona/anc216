#include <string>
#include <stack>
#include "tokenizer.hh"
#include "ast.hh"
#include "types.hh"
#include <iostream>
#include <cstring>
#include <tuple>
#include <map>
#include <sstream>
#include "console.hh"

#define expected_error_message(X) \
    X[0] == 'i' || X[0] == 'a' || X[0] == 'o' || X[0] == 'e' ? "An " + std::string(X) + " was expected" : "A " + std::string(X) + " was expected"

#define unexpected_error_message(X) \
    "Unexpected " + std::string(X) + " token"

#pragma once

/*
Grammar for parsing

*/

namespace ANC216
{
    class Parser
    {
    private:
        Tokenizer tokenizer;
        std::stack<SyntaxError> error_stack;

        void skip_line()
        {
            while (tokenizer.get_current_token() != "\n" && tokenizer.get_current_token().type != END)
                tokenizer.next_token();
        }

        AST *prog()
        {
            AST *ast = new AST(COMMAND);
            if (tokenizer.get_current_token() == "section")
            {
                ast->insert(section());
                ast->insert(prog());
                return ast;
            }

            if (tokenizer.get_current_token().type == IDENTIFIER)
            {
                if (tokenizer.get_next_token() == ":")
                {
                    ast->insert(label());
                    ast->insert(prog());
                    return ast;
                }
                ast->insert(exp_list());
                ast->insert(prog());
                return ast;
            }

            if (tokenizer.get_next_token().type == NUMBER_LITERAL || tokenizer.get_next_token().type == STRING_LITERAL || tokenizer.get_next_token().type == OPEN_ROUND_BRACKET || tokenizer.get_next_token() == "+" || tokenizer.get_next_token() == "-" || tokenizer.get_next_token() == "sizeof" || tokenizer.get_next_token() == "word" || tokenizer.get_next_token() == "byte" || tokenizer.get_next_token() == "reserve" || tokenizer.get_next_token() == "$")
            {
                ast->insert(exp_list());
                ast->insert(prog());
                return ast;
            }

            if (tokenizer.get_current_token() == "var")
            {
                ast->insert(declaration());
                ast->insert(prog());
                return ast;
            }

            if (tokenizer.get_current_token().type == INSTRUCTION)
            {
                ast->insert(instruction());
                ast->insert(prog());
            }

            if (tokenizer.get_current_token() == "global" || tokenizer.get_current_token() == "local")
            {
                ast->insert(label());
                ast->insert(prog());
                return ast;
            }

            if (tokenizer.get_current_token() == "\n")
            {
                tokenizer.next_token();
                return prog();
            }

            if (tokenizer.get_current_token().end())
                return ast;

            error_stack.push({unexpected_error_message("\"" + tokenizer.get_current_token().value + "\""), tokenizer.get_next_token()});
            tokenizer.next_token();
            return nullptr;
        }

        AST *section()
        {
            AST *ast = new AST(SECTION);
            ast->insert(new AST(tokenizer.get_current_token()));
            tokenizer.next_token();
            if (tokenizer.get_current_token() != ".")
            {
                error_stack.push({expected_error_message("\".\""), tokenizer.get_next_token()});
                tokenizer.next_token();
                return nullptr;
            }
            ast->insert(new AST(tokenizer.get_current_token()));
            tokenizer.next_token();
            if (tokenizer.get_current_token().type != IDENTIFIER)
            {
                error_stack.push({expected_error_message("identifier"), tokenizer.get_next_token()});
                tokenizer.next_token();
                return nullptr;
            }
            ast->insert(new AST(tokenizer.get_current_token()));
            tokenizer.next_token();
            if (tokenizer.get_current_token() != "\n" && tokenizer.get_current_token().type != END)
            {
                error_stack.push({"Expected the end of the line after a section", tokenizer.get_next_token()});
                tokenizer.next_token();
                return nullptr;
            }
            tokenizer.next_token();
            return ast;
        }

        AST *label()
        {
            AST *ast = new AST(LABEL);
            if (tokenizer.get_current_token() == "global" || tokenizer.get_current_token() == "local")
            {
                tokenizer.next_token();
                if (tokenizer.get_current_token().type != IDENTIFIER)
                {
                    error_stack.push({expected_error_message("identifier"), tokenizer.get_next_token()});
                    tokenizer.next_token();
                    return nullptr;
                }
            }
            ast->insert(new AST(tokenizer.get_current_token()));
            tokenizer.next_token();
            if (tokenizer.get_current_token() != ":")
            {
                error_stack.push({expected_error_message("\":\""), tokenizer.get_next_token()});
                tokenizer.next_token();
                return nullptr;
            }
            ast->insert(new AST(tokenizer.get_current_token()));
            tokenizer.next_token();
            return ast;
        }

        AST *exp_list()
        {
            AST *ast = new AST(EXPRESSION_LIST);
            if (tokenizer.get_current_token().type == STRING_LITERAL)
            {
                ast->insert(new AST(tokenizer.get_current_token()));
                tokenizer.next_token();
            }
            else if (tokenizer.get_current_token() == "reserve")
            {
                ast->insert(new AST(tokenizer.get_current_token()));
                if (!(tokenizer.get_next_token().type == NUMBER_LITERAL || tokenizer.get_next_token().type == STRING_LITERAL || tokenizer.get_next_token().type == OPEN_ROUND_BRACKET || tokenizer.get_next_token() == "+" || tokenizer.get_next_token() == "-" || tokenizer.get_next_token() == "sizeof" || tokenizer.get_next_token() == "word" || tokenizer.get_next_token() == "byte" || tokenizer.get_next_token() == "reserve" || tokenizer.get_next_token() == "$"))
                {
                    error_stack.push({expected_error_message("expression"), tokenizer.get_next_token()});
                    skip_line();
                    return nullptr;
                }
                tokenizer.next_token();
                ast->insert(expression());
            }
            else {
                ast->insert(expression());
            }

            if (tokenizer.get_current_token() == ",")
            {
                if (tokenizer.get_next_token() == "\n" || tokenizer.get_next_token().type == END)
                    return ast;
                tokenizer.next_token();
                ast->insert(exp_list());
                return ast;
            }
            if (tokenizer.get_next_token() != "\n" && tokenizer.get_next_token().type != END)
            {
                error_stack.push({expected_error_message("\",\""), tokenizer.get_current_token()});
                tokenizer.next_token();
                return nullptr;
            }
            return ast;
        }

        AST *instruction()
        {
            AST *ast = new AST(INSTRUCTION_RULE);
            ast->insert(new AST(tokenizer.get_current_token()));
            tokenizer.next_token();
            AST *res = addr();
            if (res != nullptr)
                ast->insert(res);
            tokenizer.next_token();
            return ast;
        }

        AST *addr()
        {
            if (tokenizer.get_current_token() == "\n" || tokenizer.get_current_token().type == END)
                return nullptr;
            

        }

        AST *declaration()
        {
            AST *ast = new AST(DECLARATION);
            ast->insert(new AST(tokenizer.get_current_token()));
            tokenizer.next_token();
            if (tokenizer.get_current_token().type != IDENTIFIER)
            {
                error_stack.push({expected_error_message("identifier"), tokenizer.get_next_token()});
                tokenizer.next_token();
                return nullptr;
            }
            ast->insert(new AST(tokenizer.get_current_token()));
            tokenizer.next_token();
            if (tokenizer.get_current_token() != ":")
            {
                error_stack.push({expected_error_message("\":\""), tokenizer.get_next_token()});
                tokenizer.next_token();
                return nullptr;
            }
            ast->insert(new AST(tokenizer.get_current_token()));
            tokenizer.next_token();
            if (tokenizer.get_current_token() != "byte" && tokenizer.get_current_token() != "word" && tokenizer.get_current_token().type != IDENTIFIER)
            {
                error_stack.push({expected_error_message("type"), tokenizer.get_next_token()});
                tokenizer.next_token();
                return nullptr;
            }
            ast->insert(new AST(tokenizer.get_current_token()));
            tokenizer.next_token();
            if (tokenizer.get_current_token() == "=")
            {
                ast->insert(new AST(tokenizer.get_current_token()));
                tokenizer.next_token();
                ast->insert(expression());
            }
            if (tokenizer.get_current_token() != "\n" && tokenizer.get_current_token().type != END)
            {
                error_stack.push({"Expected the end of the line after a variabile declaration", tokenizer.get_next_token()});
                tokenizer.next_token();
                return nullptr;
            }
            tokenizer.next_token();
            return ast;
        }

        AST *expression()
        {
            AST *ast = new AST(EXPRESSION);
            AST *temp;
            ast->insert(multiplication());
            while (tokenizer.get_current_token() == "+" || tokenizer.get_current_token() == "-")
            {
                ast->insert(new AST(tokenizer.get_current_token()));
                tokenizer.next_token();
                ast->insert(multiplication());
                temp = ast;
                ast = new AST(EXPRESSION);
                ast->insert(temp);
            }
            return ast;
        }

        AST *multiplication()
        {
            AST *ast = new AST(EXPRESSION);
            AST *temp;
            ast->insert(atom());
            while (tokenizer.get_current_token() == "*" || tokenizer.get_current_token() == "/")
            {
                ast->insert(new AST(tokenizer.get_current_token()));
                tokenizer.next_token();
                ast->insert(atom());
                temp = ast;
                ast = new AST(EXPRESSION);
                ast->insert(temp);
            }
            return ast;
        }

        AST *atom()
        {
            AST *ast;
            switch (tokenizer.get_current_token().type)
            {
            case OPEN_ROUND_BRACKET:
                ast = new AST(EXPRESSION);
                ast->insert(new AST(tokenizer.get_current_token()));
                tokenizer.next_token();
                if (!(tokenizer.get_next_token().type == NUMBER_LITERAL || tokenizer.get_next_token().type == STRING_LITERAL || tokenizer.get_next_token().type == OPEN_ROUND_BRACKET || tokenizer.get_next_token() == "+" || tokenizer.get_next_token() == "-" || tokenizer.get_next_token() == "sizeof" || tokenizer.get_next_token() == "word" || tokenizer.get_next_token() == "byte" || tokenizer.get_next_token() == "reserve" || tokenizer.get_next_token() == "$"))
                {
                    error_stack.push({expected_error_message("expression"), tokenizer.get_next_token()});
                    skip_line();
                    return nullptr;
                }
                ast->insert(expression());
                if (tokenizer.get_current_token() != ")")
                {
                    error_stack.push({expected_error_message("\")\""), tokenizer.get_next_token()});
                    tokenizer.next_token();
                    return nullptr;
                }
                ast->insert(new AST(tokenizer.get_current_token()));
                tokenizer.next_token();
                return ast;
            case UNARY_LEFT_OPERATOR:
                if (tokenizer.get_current_token() == "sizeof")
                {
                    ast = new AST(EXPRESSION);
                    ast->insert(new AST(tokenizer.get_current_token()));
                    if (tokenizer.get_next_token().type != IDENTIFIER)
                    {
                        error_stack.push({expected_error_message("identifier"), tokenizer.get_next_token()});
                        tokenizer.next_token();
                        return nullptr;
                    }
                    tokenizer.next_token();
                    ast->insert(new AST(tokenizer.get_current_token()));
                    tokenizer.next_token();
                    return ast;
                }
            case TYPE:
                ast = new AST(EXPRESSION);
                ast->insert(new AST(tokenizer.get_current_token()));
                tokenizer.next_token();
                ast->insert(expression());
                return ast;
            case NUMBER_LITERAL:
            case IDENTIFIER:
            case CURRENT_ADDRESS:
                ast = new AST(tokenizer.get_current_token());
                tokenizer.next_token();
                return ast;
            default:
                error_stack.push({unexpected_error_message("\"" + tokenizer.get_current_token().value + "\""), tokenizer.get_next_token()});
                tokenizer.next_token();
                return nullptr;
            }
        }

    public:
        Parser(const std::string &str)
            : tokenizer(str, error_stack)
        {
        }

        ~Parser() = default;

        inline AST *parse()
        {
            return prog();
        }

        inline std::stack<SyntaxError> &get_error_stack()
        {
            return error_stack;
        }
    };
}