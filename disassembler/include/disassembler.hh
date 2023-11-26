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

        bool analyze(std::string &out)
        {
            int current = in.get();
            if (current == EOF)
                return false;
            int next = in.get();
            if (analyze_instruction(current, next, out))
                return true;
            analyze_exp(current, next, out);
            return true;
        }

        bool analyze_instruction(int current_i, int next_i, std::string &out)
        {
            if (next_i == EOF)
                return false;
            unsigned char current = current_i;
            unsigned char next = next_i;

            std::pair<AddressingMode, size_t> adrsize;
            if ((adrsize = possible_addressing(current, next)).first == NONE)
                return false;

            auto adr = adrsize.first;
            auto size = adrsize.second;

            bool reverse = isa[next].first == "write" || isa[next].first == "store" || isa[next].first == "hwrite";

            switch (adr)
            {
            case IMPLIED_MODE:
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + "\t\t\t\t\t\; " + to_hex_string(current) + " " + to_hex_string(next) + "\n";
                return true;
            case MEMORY_RELATIVE_TO_BP_WITH_REGISTER:
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + std::string("bp + l") + X1_GET_REG(current) + "\t\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + "\n";
                return true;
            case MEMORY_RELATIVE_TO_PC_WITH_REGISTER:
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + std::string("* ") + X1_GET_REG(current) + "\t\t\t; " + signed_to_hex_string(current) + " " + to_hex_string(next) + "\n";
                return true;
            case REGISTER_ACCESS_MODE:
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + std::string(size == 1 ? "l" : "r") + X1_GET_REG(current) + "\t\t\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + "\n";
                return true;
            case REGISTER_TO_REGISTER_MODE:
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + std::string("r") + X1_GET_REG(current) + ", " + std::string("r") + X2_GET_REG(current) + "\t\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + "\n";
                return true;
            }

            unsigned char add1 = in.get();
            if (add1 == EOF)
            {
                return false;
            }

            switch (adr)
            {
            case IMMEDIATE_BYTE:
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + to_hex_string(add1) + "\t\t\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + "\n";
                return true;
            case MEMORY_RELATIVE_TO_PC:
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + std::string("* ") + signed_to_hex_string(add1) + "\t\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + "\n";
                return true;
            case MEMORY_RELATIVE_TO_BP:
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + std::string("& bp ") + signed_to_hex_string(add1) + "\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + "\n";
                return true;
            case MEMORY_RELATIVE_TO_PC_TO_REGISTER:
                if (reverse)
                {
                    out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + "* " + signed_to_hex_string(add1) + std::string(", r") + X1_GET_REG(current) + "\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + "\n";
                    return true;
                }
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + std::string("r") + X1_GET_REG(current) + ", * " + signed_to_hex_string(add1) + "\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + "\n";
                return true;
            case MEMORY_RELATIVE_TO_BP_TO_REGISTER:
                if (reverse)
                {
                    out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + "& bp " + signed_to_hex_string(add1) + std::string(", r") + X1_GET_REG(current) + "\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + "\n";
                    return true;
                }
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + std::string("r") + X1_GET_REG(current) + ", & bp " + signed_to_hex_string(add1) + "\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + "\n";
                return true;
            case MEMORY_RELATIVE_TO_PC_TO_LOW_REGISTER:
                if (reverse)
                {
                    out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + "* " + signed_to_hex_string(add1) + std::string(", l") + X1_GET_REG(current) + "\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + "\n";
                    return true;
                }
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + std::string("l") + X1_GET_REG(current) + ", * " + signed_to_hex_string(add1) + "\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + "\n";
                return true;
            case MEMORY_RELATIVE_TO_BP_TO_LOW_REGISTER:
                if (reverse)
                {
                    out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + "& bp " + signed_to_hex_string(add1) + std::string(", l") + X1_GET_REG(current) + "\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + "\n";
                    return true;
                }
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + std::string("l") + X1_GET_REG(current) + ", & bp " + signed_to_hex_string(add1) + "\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + "\n";
                return true;
            case IMMEDIATE_TO_LOW_REGISTER:
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + std::string("l") + X1_GET_REG(current) + ", " + to_hex_string(add1) + "\t\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + "\n";
                return true;
            }

            unsigned char add2 = in.get();

            if (add2 == EOF)
                return false;

            switch (adr)
            {
            case MEMORY_ABSOULTE:
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + (isa[next].first.starts_with("j") || isa[next].first == "call" ? "" : "& ") + to_hex_string((unsigned short)(add1 << 8 | add2)) + "\t\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + " " + to_hex_string(add2) + "\n";
                return true;
            case MEMORY_INDIRECT:
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + "[" + to_hex_string((unsigned short)(add1 << 8 | add2)) + "]" + "\t\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + " " + to_hex_string(add2) + "\n";
                return true;
            case IMMEDIATE_TO_REGISTER:
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + std::string("r") + X1_GET_REG(current) + ", " + to_hex_string((unsigned short)(add1 << 8 | add2)) + "\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + " " + to_hex_string(add2) + "\n";
                return true;
            case MEMORY_ABSOULTE_INDEXED:
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + "& " + to_hex_string((unsigned short)(add1 << 8 | add2)) + " + l" + X1_GET_REG(current) + "\t\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + " " + to_hex_string(add2) + "\n";
                return true;
            case MEMORY_INDIRECT_INDEXED:
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + "[" + to_hex_string((unsigned short)(add1 << 8 | add2)) + "] + l" + X1_GET_REG(current) + "\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + " " + to_hex_string(add2) + "\n";
                return true;
            case MEMORY_ABSOULTE_TO_REGISTER:
                if (reverse)
                {
                    out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + "& " + to_hex_string((unsigned short)(add1 << 8 | add2)) + std::string(", r") + X1_GET_REG(current) + "\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + " " + to_hex_string(add2) + "\n";
                    return true;
                }
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + std::string("r") + X1_GET_REG(current) + ", & " + to_hex_string((unsigned short)(add1 << 8 | add2)) + "\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + " " + to_hex_string(add2) + "\n";
                return true;
            case MEMORY_ABSOULTE_TO_LOW_REGISTER:
                if (reverse)
                {
                    out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + "& " + to_hex_string((unsigned short)(add1 << 8 | add2)) + std::string(", l") + X1_GET_REG(current) + "\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + " " + to_hex_string(add2) + "\n";
                    return true;
                }
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + std::string("l") + X1_GET_REG(current) + ", & " + to_hex_string((unsigned short)(add1 << 8 | add2)) + "\t\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + " " + to_hex_string(add2) + "\n";
                return true;
            case IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP_WITH_REGISTER:
                out += "\t" + isa[next].first + assign_ins_space(isa[next].first) + std::string("& bp + l") + X1_GET_REG(current) + ", " + (add2 == EOF ? to_hex_string(add1) : to_hex_string((unsigned short)(add1 << 8 | add2))) + "\t; " + to_hex_string(current) + " " + to_hex_string(next) + " " + to_hex_string(add1) + (add2 == EOF ? "\n" : to_hex_string(add2) + "\n");
                return true;
            }
            return false;
        }

        std::string assign_ins_space(const std::string &ins)
        {
            int space = 7 - (int)ins.size();
            std::string sp = "";
            for (int i = 0; i < space; i++)
                sp += " ";
            return sp;
        }

        std::string signed_to_hex_string(unsigned char val)
        {
            std::string str;
            if (val == 0)
                return "+0x00";
            bool neg = val & 0b1000'0000;
            val &= 0b0111'1111;

            while (val > 0)
            {
                str = std::string(1, ((val % 16 < 10) ? val % 16 + '0' : (val % 16) - 10 + 'a')) + str;
                val /= 16;
            }
            if (str.size() == 1)
                str = "0" + str;
            return neg ? "-0x" + str : "+0x" + str;
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

        std::string to_hex_string(unsigned short val)
        {
            std::string str;
            if (val == 0)
                return "0x0000";
            while (val > 0)
            {
                str = std::string(1, ((val % 16 < 10) ? val % 16 + '0' : (val % 16) - 10 + 'a')) + str;
                val /= 16;
            }
            while (str.size() != 4)
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
            else if ((current & 0b11'000'111) == 0b00'000'001)
            {
                mode = REGISTER_ACCESS_MODE;
                argsize = 2;
            }
            else if ((current & 0b11'000'111) == 0b00'000'010)
            {
                mode = REGISTER_ACCESS_MODE;
                argsize = 1;
            }
            else if ((current & 0b11'000'000) == 0b01'000'000)
            {
                mode = REGISTER_TO_REGISTER_MODE;
            }
            else if (current == 0b10'000'000)
            {
                mode = MEMORY_ABSOULTE;
                argsize = 2;
            }
            else if ((current & 0b11'000'111) == 0b10'000'001)
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
            else if ((current & 0b11'000'111) == 0b10'000'101)
            {
                mode = MEMORY_RELATIVE_TO_PC_WITH_REGISTER;
            }
            else if ((current & 0b11'000'111) == 0b10'000'110)
            {
                mode = MEMORY_RELATIVE_TO_BP_WITH_REGISTER;
            }
            else if (current == 0b10'000'010)
            {
                mode = MEMORY_INDIRECT;
                argsize = 2;
            }
            else if ((current & 0b11'000'111) == 0b10'000'011)
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
            else if ((current & 0b11'000'111) == 0b00'000'100)
            {
                mode = IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP_WITH_REGISTER;
                argsize = 1;
            }
            else if ((current & 0b11'000'111) == 0b00'000'101)
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
            else if ((current & 0b11'000'111) == 0b111)
            {
                mode = IMMEDIATE_TO_MEMORY_ABSOLUTE_INDEXED;
                argsize = 1;
            }
            else if ((current & X1_REG_MASK) == 0b11'000'000)
            {
                mode = MEMORY_ABSOULTE_TO_REGISTER;
            }
            else if ((current & X1_REG_MASK) == 0b11'000'001)
            {
                mode = IMMEDIATE_TO_REGISTER;
            }
            else if ((current & X1_REG_MASK) == 0b11'000'010)
            {
                mode = MEMORY_RELATIVE_TO_PC_TO_REGISTER;
            }
            else if ((current & X1_REG_MASK) == 0b11'000'011)
            {
                mode = MEMORY_RELATIVE_TO_BP_TO_REGISTER;
            }
            else if ((current & X1_REG_MASK) == 0b11'000'100)
            {
                mode = MEMORY_ABSOULTE_TO_LOW_REGISTER;
            }
            else if ((current & X1_REG_MASK) == 0b11'000'101)
            {
                mode = IMMEDIATE_TO_LOW_REGISTER;
            }
            else if ((current & X1_REG_MASK) == 0b11'000'110)
            {
                mode = MEMORY_RELATIVE_TO_PC_TO_LOW_REGISTER;
            }
            else if ((current & X1_REG_MASK) == 0b11'000'111)
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

        void analyze_exp(int current, int next, std::string &out)
        {
            out += "\t";
            int add1 = in.get();
            if (iswalnum((char)current) && iswalnum((char)next) && iswalnum((char)add1))
            {
                out += std::string("\"") + (char)current + (char)next + (char)add1;
                while (add1 != EOF && (iswalnum(add1 = in.get()) || (char)add1 == '!' || (char)add1 == ' ' || (char)add1 == '_' || (char)add1 == ',' || (char)add1 == '?'))
                {
                    out += std::string(1, (char)add1);
                }
                in.seekg(-1, std::ios::cur);
                out += "\"\n";
                return;
            }
            out += (next == EOF ? to_hex_string((unsigned char)current) : to_hex_string((unsigned short)((unsigned char)current << 8 | (unsigned char)next))) + "\n";
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
            while (analyze(str))
                ;
            return str;
        }
    };
}