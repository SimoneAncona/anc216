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
        REGISTER,
        NONE,
    };

    struct Token
    {
        TokenType type;
        TokenSubType subtype;
        std::string value;
        size_t column;
        size_t line;
        size_t index;
    };

    inline bool is_keyword(const std::string &str)
    {
        auto lower = str;
        std::vector<std::string> keywords = {
            "use",
            "as",
            "import",
            "if",
            "elif",
            "else",
            "end",
            "word",
            "byte",
            "sizeof", "reserve"};
        for (auto &ch : lower)
        {
            ch = std::tolower(ch);
        }
        return std::find(keywords.begin(), keywords.end(), lower) != keywords.end();
    }

    inline bool is_instruction(const std::string &str)
    {
        auto lower = str;
        std::vector<std::string> ins = {
            "kill",
            "reset",
            "cpuid",
            "syscall",
            "call",
            "ret",
            "push",
            "pop",
            "phpc",
            "popc",
            "phsr",
            "posr",
            "phsp",
            "posp",
            "phbp",
            "pobp",
            "seti",
            "sett",
            "sets",
            "clri",
            "clrt",
            "clrs",
            "clrn",
            "clro",
            "clrc",
            "ireq",
            "req",
            "write",
            "hreq",
            "hwrite",
            "read",
            "pareq",
            "cmp",
            "careq",
            "jmp",
            "jeq",
            "jz",
            "jne",
            "jnz",
            "jge",
            "jgr",
            "jle",
            "jls",
            "jo",
            "jno",
            "jn",
            "jnn",
            "inc",
            "dec",
            "add",
            "sub",
            "neg",
            "and",
            "or",
            "xor",
            "not",
            "sign",
            "shl",
            "shr",
            "par",
            "load",
            "store",
            "tran",
            "swap",
            "ldsr",
            "ldsp",
            "ldbp",
            "stsr",
            "stsp",
            "stbp",
            "trsr",
            "trsp",
            "trbp",
            "sili",
            "sihi",
            "seli",
            "sehi",
            "sbp",
            "stp",
            "tili",
            "tihi",
            "teli",
            "tehi",
            "tbp",
            "ttp",
            "lcpid",
            "tcpid",
            "time",
            "tstart",
            "tstop",
            "trt",
        };
        for (auto &ch : lower)
        {
            ch = std::tolower(ch);
        }
        return std::find(ins.begin(), ins.end(), lower) != ins.end();
    }

    inline bool is_register(const std::string &str)
    {
        if (str.size() != 2)
            return false;
        if (std::tolower(str[0]) != 'l' && std::tolower(str[0]) != 'r')
            return false;
        if (str[1] < '0' || str[1] > '9')
            return false;
        return true;
    }

    char get_next(std::string_view str, size_t &index)
    {
        if (index >= str.size())
            return EOF;
        return str[index++];
    }

    Token get_id_or_keyword(std::string_view str, size_t &index, size_t &column, size_t &line)
    {
        std::string temp = std::string(1, str[index]);
        size_t i = index;
        char next = get_next(str, index);
        while (is_alnum(next))
        {
            temp += str[index];
            next = get_next(str, index);
        }
        int type = 0;
        if (is_keyword(temp))
            type = 1;
        else if (is_instruction(temp))
            type = 2;
        else if (is_register(temp))
            type = 3;
        return {
            type == 0 ? ID : KEYWORD,
            type == 1 ? PREPROC_KEYWORD : type == 2 ? INSTRUCTION
                                      : type == 3   ? REGISTER
                                                    : NONE,
            temp,
            column, line, i};
    }

    Token get_number(std::string_view str, size_t &index, size_t &column, size_t &line)
    {
        return {};
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

            next = A216::get_next(str, index);
        }
        return tokens;
    }
}