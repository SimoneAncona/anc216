#include <fstream>
#include <iostream>
#include <string>

#include <console.hh>
#include <header.hh>
#include <isa.hh>

#define X1_REG_MASK 0b11'000'111
#define X2_REG_MASK 0b11'000'000

#define X1_GET_REG(x) (char)(((x & 0b00111000) >> 3) + '0')
#define X2_GET_REG(x) (char)(((x & 0b00000111)) + '0')

namespace ANC216
{
    class Disassembler
    {
    private:
        std::ifstream &in;
        Header header;

        void analyze(std::string &out)
        {
            unsigned char current = in.get();
            unsigned char next = in.get();
            if (analyze_instruction(current, next, out))
                return;
            analyze_exp(current, next, out);
        }

        bool analyze_instruction(unsigned char current, unsigned char next, std::string &out)
        {
            if (next == EOF)
                return false;
            std::pair<AddressingMode, size_t> adrsize;
            if ((adrsize = possible_addressing(current, next)).first == NONE)
                return false;

            auto adr = adrsize.first;
            auto size = adrsize.second;
            
            out += "\t" + isa[next].first + " ";
            switch (adr)
            {
            case IMPLIED_MODE:
                out += "\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + "\n";
                return;
            case MEMORY_RELATIVE_TO_BP_WITH_REGISTER:
                out += std::string("bp + l") + X1_GET_REG(current) + "\t; " + to_hex_string(current) + " " + to_hex_string(next) + "\n";
                return;
            case MEMORY_RELATIVE_TO_PC_WITH_REGISTER:
                out += std::string("* ") + X1_GET_REG(current) + "\t; " + to_hex_string(current) + " " + to_hex_string(next) + "\n";
                return;
            case REGISTER_ACCESS_MODE:
                out += std::string(size == 1 ? "l" : "r") + X1_GET_REG(current) + "\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + "\n";
                return;
            case REGISTER_TO_REGISTER_MODE:
                out += std::string("r") + X1_GET_REG(current) + ", " + std::string("r") + X2_GET_REG(current) + "\t; " + to_hex_string(current) + " " + to_hex_string(next) + "\n";
                return;
            }

            unsigned char add1 = in.get();
            if (add1 == EOF)
            {
                in.seekg(-1, std::ios::cur);
                return false;
            }
        }

        std::string to_hex_string(unsigned char val)
{
    std::string str;
    if (val == 0)
        return "0x00";
    while (val > 0)
    {
        str = std::string(1, ((val % 16 < 10) ? val % 16 + '0' : (val % 16) - 10 + 'a')) + str;
        val /= 16;
    }
    if (str.size() == 1)
        str = "0" + str;
    return "0x" + str;
}

        std::pair<AddressingMode, short> possible_addressing(unsigned char current, unsigned char next)
        {
            AddressingMode mode = NONE;
            size_t argsize = 0;
            if (current == 0)
            {
                mode = IMPLIED_MODE;
            }
            else if (current == 0b00'001'000)
            {
                mode = IMMEDIATE_BYTE;
                argsize = 1;
            }
            else if (current == 0b00'010'000)
            {
                mode = IMMEDIATE_WORD;
                argsize = 2;
            }
            else if (current & 0b11'000'111 == 0b00'000'001)
            {
                mode = REGISTER_ACCESS_MODE;
                argsize = 2;
            }
            else if (current & 0b11'000'111 == 0b00'000'010)
            {
                mode = REGISTER_ACCESS_MODE;
                argsize = 1;
            }
            else if (current & 0b11'000'000 == 0b01'000'000)
            {
                mode = REGISTER_TO_REGISTER_MODE;
            }
            else if (current == 0b10'000'000)
            {
                mode = MEMORY_ABSOULTE;
                argsize = 2;
            }
            else if (current & 0b11'000'111 == 0b10'000'001)
            {
                mode = MEMORY_ABSOULTE_INDEXED;
                argsize = 2;
            }
            else if (current == 0b10'000'100)
            {
                mode = MEMORY_RELATIVE_TO_PC;
                argsize = 1;
            }
            else if (current == 0b10'001'100)
            {
                mode = MEMORY_RELATIVE_TO_BP;
                argsize = 1;
            }
            else if (current & 0b11'000'111 == 0b10'000'101)
            {
                mode = MEMORY_RELATIVE_TO_PC_WITH_REGISTER;
            }
            else if (current & 0b11'000'111 == 0b10'000'110)
            {
                mode = MEMORY_RELATIVE_TO_BP_WITH_REGISTER;
            }
            else if (current == 0b10'000'010)
            {
                mode = MEMORY_INDIRECT;
                argsize = 2;
            }
            else if (current & 0b11'000'111 == 0b10'000'011)
            {
                mode = MEMORY_INDIRECT_INDEXED;
                argsize = 2;
            }
            else if (current == 0b11)
            {
                mode = IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP;
                argsize = 1;
            }
            else if (current == 0b1011)
            {
                mode = IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP;
                argsize = 2;
            }
            else if (current & 0b11'000'111 == 0b00'000'100)
            {
                mode = IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP_WITH_REGISTER;
                argsize = 1;
            }
            else if (current & 0b11'000'111 == 0b00'000'101)
            {
                mode = IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP_WITH_REGISTER;
                argsize = 2;
            }
            else if (current == 0b110)
            {
                mode = IMMEDIATE_TO_MEMORY_ABSOLUTE;
                argsize = 3;
            }
            else if (current == 0b1110)
            {
                mode = IMMEDIATE_TO_MEMORY_ABSOLUTE;
                argsize = 4;
            }
            else if (current & 0b11'000'111 == 0b111)
            {
                mode = IMMEDIATE_TO_MEMORY_ABSOLUTE_INDEXED;
                argsize = 1;
            }
            else if (current & X1_REG_MASK == 0b11'000'000)
            {
                mode = MEMORY_ABSOULTE_TO_REGISTER;
            }
            else if (current & X1_REG_MASK == 0b11'000'001)
            {
                mode = IMMEDIATE_TO_REGISTER;
            }
            else if (current & X1_REG_MASK == 0b11'000'010)
            {
                mode = MEMORY_RELATIVE_TO_PC_TO_REGISTER;
            }
            else if (current & X1_REG_MASK == 0b11'000'011)
            {
                mode = MEMORY_RELATIVE_TO_BP_TO_REGISTER;
            }
            else if (current & X1_REG_MASK == 0b11'000'100)
            {
                mode = MEMORY_ABSOULTE_TO_LOW_REGISTER;
            }
            else if (current & X1_REG_MASK == 0b11'000'101)
            {
                mode = IMMEDIATE_TO_LOW_REGISTER;
            }
            else if (current & X1_REG_MASK == 0b11'000'110)
            {
                mode = MEMORY_RELATIVE_TO_PC_TO_LOW_REGISTER;
            }
            else if (current & X1_REG_MASK == 0b11'000'111)
                mode = MEMORY_RELATIVE_TO_BP_TO_LOW_REGISTER;

            if (mode == NONE)
                return {NONE, 0};
            auto ins = isa.find(next);
            if (ins == isa.end())
                return {NONE, 0};
            if (!contains<AddressingModeFamily>(ins->second.second, get_family(mode)))
                return {NONE, 0};

            return {mode, argsize};
        }

        template <typename T>
        bool contains(std::vector<T> v, T el)
        {
            for (auto &e : v)
            {
                if (e == el)
                    return true;
            }
            return false;
        }

        void analyze_exp(unsigned char current, unsigned char next, std::string &out)
        {
        }

    public:
        Disassembler(std::ifstream &in, const std::string &header_name)
            : in(in),
              header(in)
        {
            if (!header_name.empty())
            {
                if (!header.process(header_name))
                {
                    std::cerr << RED << "Error: " << RESET << "unrecognized or corrupted header" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
        }

        ~Disassembler() = default;

        std::string disassemble()
        {
            std::string str;
            while (!in.eof())
            {
                analyze(str);
            }
        }
    };
}