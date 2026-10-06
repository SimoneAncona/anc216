#pragma once
#include <analyzer.hh>
#include <vector>
#include <stdexcept>
namespace ANC216
{
    enum HeaderType
    {
        UALF
    };
    class HeaderBuilder
    {
        std::vector<unsigned char> header;
        std::vector<Error> errors;
        void word(size_t value)
        {
            if (value > 0xffff)
                throw std::runtime_error("UALf address exceeds 16 bits");
            header.push_back(value >> 8);
            header.push_back(value);
        }
        void size()
        {
            if (header.size() > 0xffff)
                throw std::runtime_error("UALf header exceeds 16 bits");
            header[9] = header.size() >> 8;
            header[10] = header.size();
        }

    public:
        HeaderBuilder &setUALf()
        {
            header = {'U', 'A', 'L', 1, 0, 0, 1, 0, 0, 0, 11};
            return *this;
        }
        HeaderBuilder &setSymbolTableOnly()
        {
            header.at(8) = 2;
            return *this;
        }
        HeaderBuilder &setSymbolTable(std::map<std::string, Label> &labels)
        {
            size_t total = 11;
            for (const auto &[name, label] : labels)
                if (!label.is_local)
                    total += name.size() + 5;
            if (total > 0xffff)
                throw std::runtime_error("UALf symbol table is too large");
            for (const auto &[name, label] : labels)
            {
                if (label.is_local)
                    continue;
                header.insert(header.end(), name.begin(), name.end());
                header.push_back(0);
                word(label.address);
                word(total + label.address);
            }
            size();
            return *this;
        }
        HeaderBuilder &setEntryPoint(std::map<std::string, Label> &labels)
        {
            auto it = labels.find("_code");
            if (it == labels.end())
                throw std::runtime_error("UALf requires an _code entry point label");
            size_t entry = header.size() + it->second.address;
            if (entry > 0xffff)
                throw std::runtime_error("UALf entry point exceeds 16 bits");
            header[4] = entry >> 8;
            header[5] = entry;
            return *this;
        }
        std::vector<unsigned char> &get_header()
        {
            return header;
        }
    };
} // namespace ANC216
