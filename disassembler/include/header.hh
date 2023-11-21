#include <fstream>
#include <string>
#include <map>
#include <tuple>

namespace ANC216
{
    class Header
    {
    private:
        std::ifstream &input;
        std::map<std::string, int> symbols;

        bool process_ualf()
        {
            std::string magic = {(char)input.get(), (char)input.get(), (char)input.get()};
            if (magic != "UAL")
                return false;
            input.seekg(3, input.beg);
            if (input.get() != 1)
                return false;
            input.seekg(6, input.beg);
            if (input.get() != 1)
                return false;
            input.seekg(9, input.beg);
            int header_size = input.get() << 8 | input.get();

            std::string temp_symbol = "";
            char ch;
            int temp;
            while (input.tellg() < header_size)
            {
                while ((ch = input.get()) != 0)
                {
                    temp_symbol += ch;
                }
                input.get();
                input.get();
                temp = input.get() << 8 | input.get();
                symbols[temp_symbol] = temp;
            }
        }

    public:
        Header(std::ifstream &in)
            : input(in)
        {
        }

        ~Header() = default;

        inline bool process(const std::string &header_type)
        {
            if (header_type == "ualf")
                return process_ualf();
        }

        inline std::map<std::string, int> &get_symbols()
        {
            return symbols;
        }
    };
}