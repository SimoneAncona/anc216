#include "types.hh"
#include <string>
#include <tuple>
#include <cstring>
#include <stack>

namespace ANC216
{
    class Tokenizer
    {
    private:
        size_t index;
        size_t line;
        size_t column;
        Token current;
        Token next;
        const std::string &program;
        std::stack<SyntaxError> &error_stack;

        inline Token nil() const { return {"", END, index, line, column}; };

        Token eat(size_t index, size_t line, size_t column)
        {
            if (index >= program.length())
                return {"", END, index, line, column};

            for (; (program[index] == '\r' || program[index] == '\t' || program[index] == ' ') && index < program.length(); index++)
                ;

            if (index >= program.length())
                return {"", END, index, line, column};

            if (program[index] == '\n')
                return {"\n", NEW_LINE, index, line, column};

            if (program[index] == ',' || program[index] == '.' || program[index] == ':')
                return {std::string(1, program[index]), SEPARATOR, index, line, column};

            if (program[index] == ';')
                return {";", COMMENT, index, line, column};

            if (program[index] == '[')
                return {"[", OPEN_SQUARE_BRACKET, index, line, column};

            if (program[index] == ']')
                return {"]", CLOSED_SQUARE_BRACKET, index, line, column};

            if (program[index] == '(')
                return {"(", OPEN_ROUND_BRACKET, index, line, column};

            if (program[index] == ')')
                return {")", CLOSED_ROUND_BRACKET, index, line, column};

            if (program[index] == '&' || program[index] == '*' || program[index] == '+' || program[index] == '-' || program[index] == '=')
                return {std::string(1, program[index]), OTHER, index, line, column};

            if (program[index] == '$')
                return {"$", CURRENT_ADDRESS, index, line, column};

            if (std::isalpha(program[index]) || program[index] == '_')
                return eat_id_or_keyword(index, line, column);

            if (std::isdigit(program[index]))
                return eat_number(index, line, column);

            error_stack.push({"Unexpected character", {std::string(1, program[index]), OTHER, index, line, column}});
            return nil();

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
                        error_stack.push({"Unexpected character '" + std::string(1, program[i]) + "' while parsing literal number", {value, NUMBER_LITERAL, index, line, column}});
                        return nil();
                    }
                    if (program[i] != '0' && program[i] != '1')
                        break;
                    value += program[i];
                }
                return {value, NUMBER_LITERAL, index, line, column};
            }

            if (program[index] == '0' && index < program.size() && program[index + 1] == 'x')
            {
                for (size_t i = index; i < program.size(); i++)
                {
                    if ((program[i] > 'f' && program[i] < 'z') || (program[i] > 'F' && program[i] < 'Z'))
                    {
                        error_stack.push({"Unexpected character '" + std::string(1, program[i]) + "' while parsing literal number", {value, NUMBER_LITERAL, index, line, column}});
                        return nil();
                    }
                    if (program[i] < '0' || program[i] > '9' && !std::isalpha(program[i]))
                        break;
                    value += program[i];
                }
                return {value, NUMBER_LITERAL, index, line, column};
            }

            for (size_t i = index; i < program.size(); i++)
            {
                if (isalpha(program[i]))
                {
                    error_stack.push({"Unexpected character '" + std::string(1, program[i]) + "' while parsing literal number", {value, NUMBER_LITERAL, index, line, column}});
                    return nil();
                }
                if (program[i] < '0' || program[i] > '9')
                    break;
                value += program[i];
            }
            return {value, NUMBER_LITERAL, index, line, column};
        }

        Token eat_string(size_t index, size_t line, size_t column)
        {
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
                return {lower, KEYWORD, index, line, column};

            if (lower == "byte" || lower == "word")
                return {lower, TYPE, index, line, column};

            if (lower == "sizeof")
                return {lower, UNARY_LEFT_OPERATOR, index, line, column};

            if (lower == "kill" || lower == "reset" || lower == "cpuid" || lower == "syscall" || lower == "call" || lower == "ret" || lower == "push" || lower == "pop" || lower == "phpc" || lower == "popc" || lower == "phsr" || lower == "posr" || lower == "phsp" || lower == "posp" || lower == "phbp" || lower == "pobp" || lower == "seti" || lower == "sett" || lower == "sets" || lower == "clri" || lower == "clrt" || lower == "clrs" || lower == "clrn" || lower == "clro" || lower == "clrc" || lower == "ireq" || lower == "req" || lower == "write" || lower == "hreq" || lower == "hwrite" || lower == "read" || lower == "pareq" || lower == "cmp" || lower == "careq" || lower == "jmp" || lower == "jeq" || lower == "jz" || lower == "jne" || lower == "jnz" || lower == "jge" || lower == "jgr" || lower == "jle" || lower == "jls" || lower == "jo" || lower == "jno" || lower == "jn" || lower == "jnn" || lower == "inc" || lower == "dec" || lower == "add" || lower == "sub" || lower == "neg" || lower == "and" || lower == "or" || lower == "xor" || lower == "not" || lower == "sign" || lower == "shl" || lower == "shr" || lower == "par" || lower == "load" || lower == "store" || lower == "tran" || lower == "swap" || lower == "ldsr" || lower == "ldsp" || lower == "ldbp" || lower == "stsr" || lower == "stsp" || lower == "stbp" || lower == "trsr" || lower == "trsp" || lower == "trbp" || lower == "sili" || lower == "sihi" || lower == "seli" || lower == "sehi" || lower == "sbp" || lower == "stp" || lower == "tili" || lower == "tihi" || lower == "teli" || lower == "tehi" || lower == "tbp" || lower == "ttp" || lower == "lcpid" || lower == "tcpid" || lower == "time" || lower == "tstart" || lower == "tstop" || lower == "trt")
                return {lower, INSTRUCTION, index, line, column};

            if (lower.size() == 2 && (lower[0] == 'l' || lower[0] == 'r') && (lower[1] >= '0' && lower[1] <= '7'))
                return {lower, REGISTER, index, line, column};

            return {value, IDENTIFIER, index, line, column};
        }

    public:
        Tokenizer(const std::string &str, std::stack<SyntaxError> &errors)
            : program(str),
              error_stack(errors)
        {
            current = eat(0, 1, 1);
            index = current.get_last_index();
            line = current.get_last_line();
            column = current.get_last_column();
            next = eat(index, line, column);
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
            index = current.get_last_index();
            line = current.get_last_line();
            column = current.get_last_column();
            next = eat(index, line, column);
            return current;
        }
    };
}