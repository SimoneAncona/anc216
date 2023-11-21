#include <sstream>
#include <console.hh>
#include <vector>
#include <string>

#pragma once

namespace ANC216
{
    enum TokenType
    {
        IDENTIFIER,
        INSTRUCTION,
        KEYWORD,
        NUMBER_LITERAL,
        STRING_LITERAL,
        BINARY_OPERATOR,
        UNARY_LEFT_OPERATOR,
        REGISTER,
        TYPE,
        OPEN_SQUARE_BRACKET,
        CLOSED_SQUARE_BRACKET,
        OPEN_ROUND_BRACKET,
        CLOSED_ROUND_BRACKET,
        CURRENT_ADDRESS,
        SEPARATOR,
        COMMENT,
        WHITESPACE,
        NEW_LINE,
        OTHER,
        END
    };

    class Token
    {
    public:
        std::string value;
        TokenType type;
        size_t index;
        size_t line;
        size_t column;
        std::string module_name;

        Token() = default;

        Token(const std::string &value, TokenType type, size_t index, size_t line, size_t column, std::string module_name)
        {
            this->value = value;
            this->type = type;
            this->index = index;
            this->line = line;
            this->column = column;
            this->module_name = module_name;
        }

        Token(const char *value, TokenType type)
        {
            this->value = value;
            this->type = type;
            this->line = line;
            this->column = column;
        }

        ~Token() = default;

        inline bool operator==(const std::string &value)
        {
            return this->value == value;
        }

        inline bool operator==(const char *value)
        {
            return this->value == value;
        }

        inline bool end()
        {
            return type == END;
        }

        inline size_t length()
        {
            return value.length();
        }

        inline size_t get_last_index()
        {
            return value.length() + index;
        }

        inline size_t get_last_column()
        {
            if (value == "\n")
                return 1;
            if (type == STRING_LITERAL)
            {
                size_t new_line = value.find_last_of('\n');
                return value.length() - new_line == value.npos ? 0 : new_line;
            }
            return value.length() + column;
        }

        inline size_t get_last_line()
        {
            if (value == "\n")
                return line + 1;
            if (type == STRING_LITERAL)
            {
                size_t n = 0;
                for (auto ch : value)
                    if (ch == '\n') n++;
                return line + n;
            }
            return line;
        }
    };

    class Error
    {
    public:
        std::string message;
        Token token;
        bool warning;

        Error(std::string message, const Token token, bool warning = false)
        {
            this->message = message;
            this->token = token;
            this->warning = warning;
        }

        inline size_t line()
        {
            return token.line;
        }

        inline size_t column()
        {
            return token.column;
        }

        inline std::string to_string()
        {
            std::stringstream ss;
            ss  << YELLOW << "( " << RESET << token.module_name << ":" << token.line << ":" << token.column << YELLOW << " )" << "\n" 
                << (warning ? YELLOW + std::string("Warning: ") : RED + std::string("Error: ")) << RESET << "at line " << CYAN << token.line << RESET << " and column " << CYAN << token.column << RESET 
                << "\n\t" << message;
            return ss.str();
        }

        inline bool is_warning()
        {
            return warning;
        }
    };

    struct AsmFlags
    {
        std::string input_file = "";
        std::string output_file = "";
        std::string header = "";
        std::vector<std::string> import_paths = {};
        unsigned char preview : 1 = 0;
        unsigned char output_size : 1 = 0;
        unsigned char get_symbol_table : 1 = 0;
        std::string path_to_stdlib = "";
        unsigned char get_time : 1 = 0;
        std::vector<std::pair<std::string, std::string>> use_as = {};
        unsigned char suppress_warnings : 1 = 0;
    };

}