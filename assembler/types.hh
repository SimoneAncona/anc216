#pragma once
#include <sstream>

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

        Token() = default;

        Token(const std::string &value, TokenType type, size_t index, size_t line, size_t column)
        {
            this->value = value;
            this->type = type;
            this->index = index;
            this->line = line;
            this->column = column;
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
            return value == "\n" ? 1 : value.length() + column;
        }

        inline size_t get_last_line()
        {
            return value == "\n" ? line + 1 : line;
        }
    };

        class SyntaxError
    {
    public:
        std::string message;
        Token token;

        SyntaxError(std::string message, const Token token)
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

        inline std::string to_string()
        {
            std::stringstream ss;
            ss << RED << "Error: " << RESET << "at line " << CYAN << token.line << RESET << " and column " << CYAN << token.column << RESET << "\n\t" << message << "\n\t" << token.value;
            return ss.str();
        }
    };

}