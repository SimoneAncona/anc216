#pragma once
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace ANC216
{
    class Header
    {
        std::ifstream &input;
        std::map<std::string, int> symbols;

    public:
        explicit Header(std::ifstream &in) : input(in)
        {
        }
        bool process(const std::string &type)
        {
            if (type != "ualf")
                return false;
            std::vector<unsigned char> h(11);
            if (!input.read(reinterpret_cast<char *>(h.data()), h.size()))
                return false;
            if (h[0] != 'U' || h[1] != 'A' || h[2] != 'L' || h[3] != 1 || h[6] != 1 || h[8] > 2)
                return false;
            unsigned size = (unsigned(h[9]) << 8) | h[10];
            if (size < 11)
                return false;
            unsigned used = 11;
            while (used < size)
            {
                std::string name;
                int ch;
                do
                {
                    if (used == size || (ch = input.get()) == EOF)
                        return false;
                    ++used;
                    if (ch)
                        name += char(ch);
                } while (ch);
                if (name.empty() || size - used < 4)
                    return false;
                unsigned char address[4];
                if (!input.read(reinterpret_cast<char *>(address), 4))
                    return false;
                used += 4;
                symbols[name] = (unsigned(address[2]) << 8) | address[3];
            }
            return true;
        }
        std::map<std::string, int> &get_symbols()
        {
            return symbols;
        }
    };
} // namespace ANC216
