#include "types.hh"
#include <string>
#include <tuple>
#include <cstring>
#include <stack>
#include <vector>

namespace ANC216
{
    class Tokenizer
    {
    private:
        Token current;
        Token next;
        std::string program;
        std::vector<Error> &error_stack;
        std::vector<Token> tokens;
        std::string module_name;
        size_t i;

        inline Token nil(size_t index, size_t line, size_t column) const { return {"", END, index, line, column, module_name}; };

        Token eat(size_t index, size_t line, size_t column)
        {
            if (index >= program.length())
                return {"", END, index, line, column, module_name};

            for (; (program[index] == '\r' || program[index] == '\t' || program[index] == ' ') && index < program.length(); index++)
                ;

            if (index >= program.length())
                return {"", END, index, line, column, module_name};

            if (program[index] == '\n')
                return {"\n", NEW_LINE, index, line, column, module_name};

            if (program[index] == ',' || program[index] == '.' || program[index] == ':')
                return {std::string(1, program[index]), SEPARATOR, index, line, column, module_name};

            if (program[index] == ';')
                return {";", COMMENT, index, line, column, module_name};

            if (program[index] == '[')
                return {"[", OPEN_SQUARE_BRACKET, index, line, column, module_name};

            if (program[index] == ']')
                return {"]", CLOSED_SQUARE_BRACKET, index, line, column, module_name};

            if (program[index] == '(')
                return {"(", OPEN_ROUND_BRACKET, index, line, column, module_name};

            if (program[index] == ')')
                return {")", CLOSED_ROUND_BRACKET, index, line, column, module_name};

            if (program[index] == '&' || program[index] == '*' || program[index] == '+' || program[index] == '-' || program[index] == '=' || program[index] == '!')
                return {std::string(1, program[index]), OTHER, index, line, column, module_name};

            if (program[index] == '$')
                return {"$", CURRENT_ADDRESS, index, line, column, module_name};

            if (std::isalpha(program[index]) || program[index] == '_')
                return eat_id_or_keyword(index, line, column);

            if (std::isdigit(program[index]))
                return eat_number(index, line, column);

            if (program[index] == '"' || program[index] == '\'')
                return eat_string(index, line, column);

            error_stack.push_back({"Unexpected character", {std::string(1, program[index]), OTHER, index, line, column, module_name}});
            return nil(index, line, column);
        }

        Token eat_number(size_t index, size_t line, size_t column)
        {
            std::string value = "";
            if (program[index] == '0' && index < program.size() && program[index + 1] == 'b')
            {
                for (size_t i = index; i < program.size(); i++)
                {
                    if (std::isalpha(program[i]) || (program[i] > '1' && program[i] <= '9'))
                    {
                        error_stack.push_back({"Unexpected character '" + std::string(1, program[i]) + "' while parsing literal number", {value, NUMBER_LITERAL, index, line, column, module_name}});
                        return nil(index, line, column);
                    }
                    if (program[i] != '0' && program[i] != '1')
                        break;
                    value += program[i];
                }
                return {value, NUMBER_LITERAL, index, line, column, module_name};
            }

            if (program[index] == '0' && index < program.size() && program[index + 1] == 'x')
            {
                for (size_t i = index; i < program.size(); i++)
                {
                    if ((program[i] > 'f' && program[i] < 'z') || (program[i] > 'F' && program[i] < 'Z'))
                    {
                        error_stack.push_back({"Unexpected character '" + std::string(1, program[i]) + "' while parsing literal number", {value, NUMBER_LITERAL, index, line, column, module_name}});
                        return nil(index, line, column);
                    }
                    if (program[i] < '0' || program[i] > '9' && !std::isalpha(program[i]))
                        break;
                    value += program[i];
                }
                return {value, NUMBER_LITERAL, index, line, column, module_name};
            }

            for (size_t i = index; i < program.size(); i++)
            {
                if (isalpha(program[i]))
                {
                    error_stack.push_back({"Unexpected character '" + std::string(1, program[i]) + "' while parsing literal number", {value, NUMBER_LITERAL, index, line, column, module_name}});
                    return nil(index, line, column);
                }
                if (program[i] < '0' || program[i] > '9')
                    break;
                value += program[i];
            }
            return {value, NUMBER_LITERAL, index, line, column, module_name};
        }

        Token eat_string(size_t index, size_t line, size_t column)
        {
            char ending = program[index];
            std::string value = std::string(1, ending);
            bool escape = false;

            for (size_t i = index + 1; !(program[i] == ending && !escape); i++)
            {
                if (i >= program.size())
                {
                    error_stack.push_back({"Unexpected the end of the file", {value, STRING_LITERAL, index, line, column, module_name}});
                    return nil(index, line, column);
                }
                if (escape)
                {
                    switch (program[i])
                    {
                    case 'n':
                        value += '\n';
                    case 't':
                        value += '\t';
                    case '0':
                        value += '\0';
                    case '\\':
                        value += '\\';
                    default:
                        value += '\\';
                        value += program[i];
                    }
                    escape = false;
                    continue;
                }
                if (program[i] == '\\')
                {
                    escape = true;
                    continue;
                }
                value += program[i];
                escape = false;
            }

            value += std::string(1, ending);

            return {value, STRING_LITERAL, index, line, column, module_name};
        }

        Token eat_id_or_keyword(size_t index, size_t line, size_t column)
        {
            std::string value = "";
            std::string lower;

            for (size_t i = index; i < program.size() && (std::isalnum(program[i]) || program[i] == '_'); i++)
                value += program[i];

            for (char c : value)
                lower += tolower(c);

            if (lower == "use" || lower == "as" || lower == "import" || lower == "if" || lower == "endif" || lower == "elif" || lower == "org" || lower == "section" || lower == "local" || lower == "global" || lower == "structure" || lower == "var" || lower == "reserve")
                return {lower, KEYWORD, index, line, column, module_name};

            if (lower == "byte" || lower == "word")
                return {lower, TYPE, index, line, column, module_name};

            if (lower == "sizeof")
                return {lower, UNARY_LEFT_OPERATOR, index, line, column, module_name};

            if (lower == "kill" || lower == "reset" || lower == "cpuid" || lower == "syscall" || lower == "call" || lower == "ret" || lower == "push" || lower == "pop" || lower == "phpc" || lower == "popc" || lower == "phsr" || lower == "posr" || lower == "phsp" || lower == "posp" || lower == "phbp" || lower == "pobp" || lower == "seti" || lower == "sett" || lower == "sets" || lower == "clri" || lower == "clrt" || lower == "clrs" || lower == "clrn" || lower == "clro" || lower == "clrc" || lower == "ireq" || lower == "req" || lower == "write" || lower == "hreq" || lower == "hwrite" || lower == "read" || lower == "pareq" || lower == "cmp" || lower == "careq" || lower == "jmp" || lower == "jeq" || lower == "jz" || lower == "jne" || lower == "jnz" || lower == "jge" || lower == "jgr" || lower == "jle" || lower == "jls" || lower == "jo" || lower == "jno" || lower == "jn" || lower == "jnn" || lower == "inc" || lower == "dec" || lower == "add" || lower == "sub" || lower == "neg" || lower == "and" || lower == "or" || lower == "xor" || lower == "not" || lower == "sign" || lower == "shl" || lower == "shr" || lower == "par" || lower == "load" || lower == "store" || lower == "tran" || lower == "swap" || lower == "ldsr" || lower == "ldsp" || lower == "ldbp" || lower == "stsr" || lower == "stsp" || lower == "stbp" || lower == "trsr" || lower == "trsp" || lower == "trbp" || lower == "sili" || lower == "sihi" || lower == "seli" || lower == "sehi" || lower == "sbp" || lower == "stp" || lower == "tili" || lower == "tihi" || lower == "teli" || lower == "tehi" || lower == "tbp" || lower == "ttp" || lower == "lcpid" || lower == "tcpid" || lower == "time" || lower == "tstart" || lower == "tstop" || lower == "trt")
                return {lower, INSTRUCTION, index, line, column, module_name};

            if (lower.size() == 2 && (lower[0] == 'l' || lower[0] == 'r') && (lower[1] >= '0' && lower[1] <= '7'))
                return {lower, REGISTER, index, line, column, module_name};

            return {value, IDENTIFIER, index, line, column, module_name};
        }

        void remove_comments()
        {
            std::string no_comments;
            char open_string = 0;
            bool escape = false;
            bool comment = false;
            for (auto ch : program)
            {
                if (open_string)
                {
                    no_comments.push_back(ch);
                    if (ch == open_string && !escape)
                    {
                        open_string = 0;
                        escape = false;
                        continue;
                    }
                    if (ch == '\\')
                    {
                        escape = true;
                        continue;
                    }
                    escape = false;
                    continue;
                }
                if (ch == ';')
                {
                    comment = true;
                    continue;
                }
                if (ch == '\n')
                {
                    comment = false;
                }
                if (!comment)
                {
                    no_comments.push_back(ch);
                }
            }
            program = no_comments;
        }

        void tokenize()
        {
            Token token = eat(0, 1, 1);
            size_t index = token.get_last_index();
            size_t line = token.get_last_line();
            size_t column = token.get_last_column();
            tokens.push_back(token);
            while (token.type != END)
            {
                token = eat(index, line, column);
                index = token.get_last_index();
                line = token.get_last_line();
                column = token.get_last_column();
                tokens.push_back(token);
            }
        }

    public:
        Tokenizer(const std::string &str, std::vector<Error> &errors, std::string module_name = "_main")
            : error_stack(errors)
        {
            program = str;
            this->module_name = module_name;
            remove_comments();
            tokenize();
            i = 0;
            current = tokens[0];
            if (tokens.size() > 1)
            {
                next = tokens[1];
                return;
            }
            next = nil(current.get_last_index(), current.get_last_line(), current.get_last_column());
        }

        inline Token get_current_token()
        {
            return current;
        }

        inline Token get_next_token()
        {
            return next;
        }

        inline Token next_token()
        {
            current = next;
            i++;
            if (i >= tokens.size() - 1)
            {
                next = nil(current.get_last_index(), current.get_last_line(), current.get_last_column());
                return current;
            }
            next = tokens[i + 1];
            return current;
        }

        inline void set_index(size_t index)
        {
            i = index;
            if (i >= tokens.size())
            {
                current = nil(current.get_last_index(), current.get_last_line(), current.get_last_column());
                next = nil(next.get_last_index(), next.get_last_line(), next.get_last_column());
                return;
            }
            current = tokens[i];
            if (i >= tokens.size() - 1)
            {
                next = nil(current.get_last_index(), current.get_last_line(), current.get_last_column());
                return;
            }
            next = tokens[i + 1];
        }

        inline void remove_current_token()
        {
            if (i < tokens.size())
            {
                tokens.erase(tokens.begin() + i);
                if (i >= tokens.size())
                {
                    current = nil(current.get_last_index(), current.get_last_line(), current.get_last_column());
                    next = nil(next.get_last_index(), next.get_last_line(), next.get_last_column());
                    return;
                }
                current = tokens[i];
                if (i >= tokens.size() - 1)
                {
                    next = nil(current.get_last_index(), current.get_last_line(), current.get_last_column());
                    return;
                }
                next = tokens[i + 1];
            }
        }

        inline void set_current_token(Token token)
        {
            if (i < tokens.size())
            {
                current = token;
                tokens[i] = token;
            }
        }

        inline size_t get_index()
        {
            return i;
        }

        inline std::vector<Token> &get_tokens()
        {
            return tokens;
        }

        inline void unshift_tokens(std::vector<Token> &token_vector)
        {
            for (size_t index = 0; index < token_vector.size(); index++)
            {
                if (token_vector[index].type == END)
                    return;
                i++;
                tokens.insert(tokens.begin() + index, token_vector[index]);
            }
            
        }
    };
}