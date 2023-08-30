#include <vector>
#include <string>

#pragma once

namespace A216
{
    enum TokenType
    {
        KEYWORD,
        ID,
        NUM,
        SEPARATOR,
        DOT_ACCESS,
        EOL, // End Of Line
        OTHER,
    };

    enum TokenSubType
    {
        DEC_NUM,
        HEX_NUM,
        BIN_NUM,
        OCT_NUM,
        PREPROC_KEYWORD,
        INSTRUCTION,
        NONE,
    };

    struct Token
    {
        TokenType type;
        TokenSubType subtype;
        std::string value;
        int column;
        int line;
        int index;
    };

    std::vector<Token> tokenize(const std::string &str)
    {
        size_t index = 0;
        size_t column = 0;
        size_t line = 0;
        std::vector<Token> tokens;
        char next = get_next(str, index);
        while (next != EOF)
        {
            if (is_alpha(next))
            {
                tokens.push_back(get_id_or_keyword(str, index, column, line));
                continue;
            }

            if (is_digit(next))
            {
                tokens.push_back(get_number(str, index, column, line));
                continue;
            }

            next = get_next(str, index);
        }
        return tokens;
    }

    Token get_id_or_keyword(std::string_view, size_t &index, size_t &column, size_t &line)
    {
    }

    Token get_number(std::string_view, size_t &index, size_t &column, size_t &line)
    {
    }

    inline bool is_alpha(char ch)
    {
        return ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z';
    }

    inline bool is_digit(char ch)
    {
        return ch >= '0' && ch <= '9';
    }

    inline bool is_alnum(char ch)
    {
        return is_alpha(ch) || is_digit(ch);
    }

    char get_next(std::string_view str, size_t &index)
    {
        if (index >= str.size())
            return EOF;
        return str[index++];
    }
}