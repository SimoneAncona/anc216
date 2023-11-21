#include <fstream>
#include <iostream>

#include <console.hh>
#include <header.hh>

namespace ANC216
{
    class Disassembler
    {
    private:
        std::ifstream &in;
        Header header;

    public:
        Disassembler(std::ifstream &in, const std::string &header_name)
            :in(in),
            header(in)
        {
            if (!header_name.empty())
            {
                if(!header.process(header_name))
                {
                    std::cerr << RED << "Error: " << RESET << "unrecognized or corrupted header" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
        }

        ~Disassembler() = default;
    };
}