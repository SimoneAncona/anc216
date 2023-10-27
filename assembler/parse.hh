#include <string>
#include <stack>
#include "ast.hh"
#include "types.hh"
#include <iostream>
#include <cstring>
#include <tuple>

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
        std::string program;
        size_t count;
        AST syntax_tree;
        AST *last_node;
        Token current_token;
        size_t column;
        size_t line;

        Token next_token()
        {
            std::pair<Token, size_t> next = get_next_token();
            current_token = next.first;
            count = next.second;
            line = current_token.line;
            column += current_token.length();
            return current_token;
        }

        std::pair<Token, size_t> get_next_token()
        {
            size_t temp_count = count;
            size_t temp_line = line;
            size_t temp_column = column;

            if (temp_count >= program.size())
                return {{"", END}, temp_count};
            while (program[temp_count] == ' ' || program[temp_count] == '\t')
            {
                temp_count++;
                if (temp_count >= program.size())
                    return {{"", END}, temp_count};
            }

            if (program[temp_count] == '\n')
            {
                while (program[temp_count] == '\n')
                {
                    temp_count++;
                    temp_line++;
                    temp_column = 0;
                }
                return {{"\n", NEW_LINE, temp_line - 1, temp_column}, temp_count};
            }

            if (std::isdigit(program[temp_count]))
                return {get_next_number_literal(temp_count, temp_line, temp_column), temp_count};

            if (program[temp_count] == '_' || isalpha(program[temp_count]))
                return {get_next_id_or_keyword(temp_count, temp_line, temp_column), temp_count};

            if (program[temp_count] == '-' || program[temp_count] == '+' || program[temp_count] == '/' || program[temp_count] == '*' || program[temp_count] == '=' || program[temp_count] == '<' || program[temp_count] == '>')
                return {{std::string(1, program[temp_count]), OPERATOR, temp_line, temp_column}, temp_count + 1};

            if (program[temp_count] == ',' || program[temp_count] == '.' || program[temp_count] == ':')
                return {{std::string(1, program[temp_count]), SEPARATOR, temp_line, temp_column}, temp_count + 1};

            error_stack.push({"Unexpected character", {std::string(1, program[temp_count]), OTHER, temp_line, temp_column}});
            return {{"", END}, temp_count};
        }

        Token get_next_id_or_keyword(size_t &count, size_t temp_line, size_t temp_column)
        {
            std::string value = "";
            std::string lower;
            for (; count < program.size() && (std::isalnum(program[count]) || program[count] == '_'); count++)
                value += program[count];

            for (char c : value)
                lower += tolower(c);

            if (lower == "use" || lower == "as" || lower == "import" || lower == "if" || lower == "endif" || lower == "elif" || lower == "org" || lower == "section" || lower == "local" || lower == "global" || lower == "struct" || lower == "var")
                return {lower, KEYWORD, temp_line, temp_column};

            if (lower == "byte" || lower == "word")
                return {lower, TYPE, temp_line, temp_column};

            if (lower == "kill" || lower == "reset" || lower == "cpuid" || lower == "syscall" || lower == "call" || lower == "ret" || lower == "push" || lower == "pop" || lower == "phpc" || lower == "popc" || lower == "phsr" || lower == "posr" || lower == "phsp" || lower == "posp" || lower == "phbp" || lower == "pobp" || lower == "seti" || lower == "sett" || lower == "sets" || lower == "clri" || lower == "clrt" || lower == "clrs" || lower == "clrn" || lower == "clro" || lower == "clrc" || lower == "ireq" || lower == "req" || lower == "write" || lower == "hreq" || lower == "hwrite" || lower == "read" || lower == "pareq" || lower == "cmp" || lower == "careq" || lower == "jmp" || lower == "jeq" || lower == "jz" || lower == "jne" || lower == "jnz" || lower == "jge" || lower == "jgr" || lower == "jle" || lower == "jls" || lower == "jo" || lower == "jno" || lower == "jn" || lower == "jnn" || lower == "inc" || lower == "dec" || lower == "add" || lower == "sub" || lower == "neg" || lower == "and" || lower == "or" || lower == "xor" || lower == "not" || lower == "sign" || lower == "shl" || lower == "shr" || lower == "par" || lower == "load" || lower == "store" || lower == "tran" || lower == "swap" || lower == "ldsr" || lower == "ldsp" || lower == "ldbp" || lower == "stsr" || lower == "stsp" || lower == "stbp" || lower == "trsr" || lower == "trsp" || lower == "trbp" || lower == "sili" || lower == "sihi" || lower == "seli" || lower == "sehi" || lower == "sbp" || lower == "stp" || lower == "tili" || lower == "tihi" || lower == "teli" || lower == "tehi" || lower == "tbp" || lower == "ttp" || lower == "lcpid" || lower == "tcpid" || lower == "time" || lower == "tstart" || lower == "tstop" || lower == "trt")
                return {lower, INSTRUCTION, temp_line, temp_column};

            return {value, IDENTIFIER, temp_line, temp_column};
        }

        Token get_next_number_literal(size_t &count, size_t temp_line, size_t temp_column)
        {
            std::string value = "";
            if (program[count] == '0' && count < program.size() && program[count + 1] == 'b')
            {
                for (count += 2; count < program.size(); count++)
                {
                    if (std::isalpha(program[count]) || (program[count] > '1' && program[count] <= '9'))
                    {
                        error_stack.push({"Unexpected character in binary number literal", {std::string(1, program[count]), OTHER, temp_line, temp_column + count}});
                        return {"", END};
                    }
                    if (program[count] != '0' && program[count] != '1')
                        break;
                    value += program[count];
                }
                return {value, NUMBER_LITERAL, temp_line, temp_column};
            }

            if (program[count] == '0' && count < program.size() && program[count + 1] == 'x')
            {
                for (count += 2; count < program.size(); count++)
                {
                    if ((program[count] > 'f' && program[count] < 'z') || (program[count] > 'F' && program[count] < 'Z'))
                    {
                        error_stack.push({"Unexpected character in hexadecimal number literal", {std::string(1, program[count]), OTHER, temp_line, temp_column + count}});
                        return {"", END};
                    }
                    if (program[count] < '0' || program[count] > '9' && !std::isalpha(program[count]))
                        break;
                    value += program[count];
                }
                return {value, NUMBER_LITERAL, temp_line, temp_column};
            }

            for (; count < program.size(); count++)
            {
                if (isalpha(program[count]))
                {
                    error_stack.push({"Unexpected character in number literal", {std::string(1, program[count]), OTHER, temp_line, temp_column + count}});
                    return {"", END};
                }
                if (program[count] < '0' || program[count] > '9')
                    break;
                value += program[count];
            }
            return {value, NUMBER_LITERAL, temp_line, temp_column};
        }

        void get_next_string_literal()
        {
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
                if (get_next_token().first == ":")
                    return label();
                return line_expression();
            }

            if (next.type == INSTRUCTION)
                return instruction();

            if (next.type == NUMBER_LITERAL || next.type == STRING_LITERAL || next.type == OPERATOR)
                return line_expression();

            if (get_next_token().first == "\n")
            {
                last_node = current_node;
                command();
            }

            if (get_next_token().first.end())
                return;

            std::cout << "no" << std::endl;
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
                if (get_next_token().first != "as")
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
            AST *current_node = last_node;
            Token next = next_token();
            last_node = last_node->insert_non_terminal(DECLARATION);

            last_node->insert_terminal(current_token);

            if (next.type != IDENTIFIER)
            {
                error_stack.push({expected_error_message("identifier"), next});
                return;
            }
            last_node->insert_terminal(next);

            next = next_token();
            if (next != ":")
            {
                error_stack.push({expected_error_message("\":\""), next});
                return;
            }
            
            last_node->insert_terminal(next);

            next = next_token();
            if (next.type != IDENTIFIER && next.type != TYPE)
            {
                error_stack.push({expected_error_message("type"), next});
                return;
            }

            last_node->insert_terminal(next);

            if (get_next_token().first == "=")
            {
                next = next_token();
                last_node->insert_terminal(next);

                next = next_token();
                if (next.type != IDENTIFIER && next.type != NUMBER_LITERAL && next.type != STRING_LITERAL && next.type != OPERATOR && next != "(")
                {
                    error_stack.push({expected_error_message("expression"), next});
                    return;
                }
                expression();
            }

            last_node = current_node;
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
        Parser(const std::string &program)
            : syntax_tree(COMMAND)
        {
            this->program = program;
            count = 0;
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