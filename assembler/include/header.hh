#include <vector>
#include <analyzer.hh>

#pragma once

namespace ANC216
{
    enum HeaderType
    {
        UALF
    };

    class HeaderBuilder
    {
    private:
        HeaderType type;
        std::vector<Error> errors;
        std::vector<unsigned char> header;

    public:
        HeaderBuilder() = default;
        ~HeaderBuilder() = default;

        HeaderBuilder &setUALf()
        {
            type = UALF;
            header = {'U', 'A', 'L', /*Verion*/ 0x01, /*Entry point*/ 0x00, 0x00, /*Arch*/ 0x01,
                      /*Flags*/ 0, /*Type*/ 0, /*Header size*/ 0x00, 0x09};
            return *this;
        }

        HeaderBuilder &setSymbolTableOnly()
        {
            header[8] = 0x02;
            return *this;
        }

        HeaderBuilder &setEntryPoint(std::map<std::string, Label> &labels)
        {
            if (labels.find("_code") == labels.end())
            {
                errors.push_back({"Cannot find _code entry point. For the UALf format is mandatory to define the '_code' label", {}});
                return *this;
            }
            return *this;
        }

        HeaderBuilder &setSymbolTable(std::map<std::string, Label> &labels)
        {
            return *this;
        }

        std::vector<unsigned char> &get_header()
        {
            return header;
        }
    };
}