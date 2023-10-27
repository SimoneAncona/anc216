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
        OPERATOR,
        NEW_LINE,
        END,
        OTHER
    };

    class Token
    {
    public:
        std::string value;
        TokenType type;
        size_t line;
        size_t column;

        Token() = default;

        Token(const std::string &value, TokenType type, size_t line, size_t column)
        {
            this->value = value;
            this->type = type;
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
            return this->type == END;
        }

        inline size_t length()
        {
            return this->value.length();
        }
    };
}