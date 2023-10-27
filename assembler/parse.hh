#include <string>
#include <stack>
#include "ast.hh"
#include "types.hh"

#define expected_error_message(X) \
    X[0] == 'i' || X[0] == 'a' || X[0] == 'o' || X[0] == 'e' ? "An " + std::string(X) + " was expected" : "A " + std::string(X) + " was expected"

#pragma once

/*
Grammar for parsing

C ::= D | L | DE | I | "\n" C | "" | S | P | ST
P ::= use id | use id as E | import id | org E | if ...
S ::= section .id
ST ::= struct id : T | ST , id : T | ST "\n" id : T
L ::= id : C | local id : C | global id : C
D ::= var id : T = E | var id : T
T ::= byte | word | id
I ::= Instr Addr
Addr ::= "" |
Instr ::= load | store | add ...
DE ::= DE , E | DE , reserve E, E | E
E ::= value | E Bop1 E | Uop E | (E) | id | T E | sizeof id | $
Bop1 ::= + | -
Bop2 ::= * | /
Uop ::= + | -
*/

namespace ANC216
{
    class SyntaxError
    {
    public:
        std::string message;
        Token token;

        SyntaxError(std::string message, const Token &token)
        {
            this->message = message;
            this->token = token;
        }

        inline size_t line()
        {
            return token.line;
        }

        inline size_t column()
        {
            return token.column;
        }
    };

    class Parser
    {
    private:
        std::stack<SyntaxError> error_stack;
        const std::string &program;
        size_t count;
        AST syntax_tree;
        AST *last_node;
        Token current_token;
        size_t column;
        size_t line;

        Token next_token()
        {
            current_token = get_next_token();
            count += current_token.length();
            line = current_token.line;
            column = current_token.column;
        }

        Token get_next_token()
        {
            size_t temp_count = count;
            size_t temp_line = line;
            size_t temp_column = column;

            if (temp_count >= program.length())
                return {"", END};
            while (program[temp_count] == ' ' || program[temp_count] == '\t')
            {
                temp_count++;
                if (temp_count >= program.length())
                    return {"", END};
            }

            if (program[temp_count] == '\n')
            {
                while (program[temp_count] == '\n')
                {
                    temp_count++;
                    line++;
                }
                return {"\n", NEW_LINE};
            }

            if (std::isdigit(program[temp_count]))
            {

            }

            if (program[temp_count] == '-' || program[temp_count] == '+' || program[temp_count] == '/' || program[temp_count] == '*')
                return {std::string(1, program[temp_count]), OPERATOR, temp_line, temp_column};

            error_stack.push({"Unexpected character", {std::string(1, program[temp_count]), OTHER, temp_line, temp_column}});
            return {"", END};
        }

        // C ::= D | L | DE | I | "\n" C | "" | S | P | ST
        void command()
        {
            AST *current_node = last_node;
            Token next = next_token();
            if (next.end())
                return;

            if (next == "var")
            {
                last_node = current_node;
                decl();
            }

            if (next == "struct")
            {
                struct_def();
                last_node = current_node;
            }

            if (next == "global" || next == "local")
            {
                label();
                last_node = current_node;
            }

            if (next == "use" || next == "import" || next == "org" || next == "if" || next == "else")
            {
                preprocessor();
                last_node = current_node;
            }

            if (next.type == IDENTIFIER)
            {
                if (get_next_token() == ":")
                    return label();
                return line_expression();
            }

            if (next.type == INSTRUCTION)
                return instruction();

            if (next.type == NUMBER_LITERAL || next.type == STRING_LITERAL || next.type == OPERATOR)
                return line_expression();

            if (get_next_token() == "\n")
            {
                last_node = current_node;
                command();
            }

            if (get_next_token().end())
                return;
        }

        // P ::= use id | use id as E | import id | org E | if E then C endif | else if...
        void preprocessor()
        {
            AST *current_node = last_node;
            Token next;
            last_node = last_node->insert_non_terminal(PREPROCESSOR);

            if (current_token == "use")
            {
                last_node->insert_terminal(current_token);
                next = next_token();
                if (next.type != IDENTIFIER)
                {
                    error_stack.push({expected_error_message("identifier"), next});
                    return;
                }
                last_node->insert_terminal(next);
                if (get_next_token() != "as")
                    return;

                next = next_token();
                last_node->insert_terminal(next);

                next = next_token();
                if (next.type != IDENTIFIER && next.type != NUMBER_LITERAL && next.type != STRING_LITERAL && next.type != OPERATOR && next != "(")
                {
                    error_stack.push({expected_error_message("expression"), next});
                    return;
                }
                expression();
                last_node = current_node;
                return;
            }
        }

        void decl()
        {
        }

        void label()
        {
        }

        void struct_def()
        {
        }

        void line_expression()
        {
        }

        void instruction()
        {
        }

        void expression()
        {
        }

    public:
        Parser(const std::string &p) 
            : program(p)
        {
            last_node = &syntax_tree;
            this->line = 0;
            this->column = 0;
        }

        ~Parser() = default;

        AST parse()
        {
            command();
            return syntax_tree;
        }

        std::stack<SyntaxError> get_error_stack()
        {
            return error_stack;
        }
    };
}