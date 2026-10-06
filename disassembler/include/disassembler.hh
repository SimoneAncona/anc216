#pragma once
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <utility>
#include "header.hh"
#include "../../common/encoding.hh"

namespace ANC216
{
    class Disassembler
    {
        std::vector<uint8_t> bytes;
        static std::string hex(unsigned value, unsigned width = 2)
        {
            std::ostringstream s;
            s << "0x" << std::hex << std::setfill('0') << std::setw(width) << value;
            return s.str();
        }
        static std::string offset(uint8_t value)
        {
            int n = static_cast<int8_t>(value);
            return std::string(n < 0 ? "- " : "+ ") + hex(n < 0 ? -n : n);
        }

    public:
        explicit Disassembler(std::vector<uint8_t> data) : bytes(std::move(data))
        {
        }
        Disassembler(std::ifstream &in, const std::string &header_name)
        {
            if (!header_name.empty())
            {
                Header header(in);
                if (!header.process(header_name))
                    throw std::runtime_error("Unrecognized or corrupted header");
            }
            bytes.assign(std::istreambuf_iterator<char>(in), {});
        }
        std::string disassemble()
        {
            using namespace anc216_isa;
            std::string out;
            for (size_t i = 0; i < bytes.size();)
            {
                Encoding e = decode(bytes[i]);
                if (i + 1 >= bytes.size() || !valid(bytes[i + 1], e) || i + 2 + e.size > bytes.size())
                {
                    out += "\tbyte " + hex(bytes[i++]) + "\n";
                    continue;
                }
                const auto op = bytes[i + 1];
                const size_t a = i + 2;
                auto word = [&](size_t at)
                {
                    return (unsigned(bytes[at]) << 8) | bytes[at + 1];
                };
                auto value = [&](size_t at)
                {
                    return e.width == 1 ? "byte " + hex(bytes[at]) : "word " + hex(word(at), 4);
                };
                const std::string r = std::string(e.width == 1 ? "l" : "r") + std::to_string(e.reg);
                const std::string l = "l" + std::to_string(e.reg);
                std::string operand;
                switch (e.mode)
                {
                case Mode::implied:
                    break;
                case Mode::immediate:
                    operand = value(a);
                    break;
                case Mode::reg:
                    operand = r;
                    break;
                case Mode::regs:
                    operand = r + ", r" + std::to_string(e.source);
                    break;
                case Mode::absolute:
                    operand = "& " + hex(word(a), 4);
                    break;
                case Mode::indexed:
                    operand = "& " + hex(word(a), 4) + " + " + l;
                    break;
                case Mode::indirect:
                    operand = "[" + hex(word(a), 4) + "]";
                    break;
                case Mode::indirect_indexed:
                    operand = "[" + hex(word(a), 4) + "] + " + l;
                    break;
                case Mode::pc:
                    operand = "* " + offset(bytes[a]);
                    break;
                case Mode::bp:
                    operand = "& bp " + offset(bytes[a]);
                    break;
                case Mode::pc_reg:
                    operand = "* " + l;
                    break;
                case Mode::bp_reg:
                    operand = "& bp + " + l;
                    break;
                case Mode::store_bp:
                    operand = "& bp " + offset(bytes[a]) + ", " + value(a + 1);
                    break;
                case Mode::store_bp_reg:
                    operand = "& bp + " + l + ", " + value(a);
                    break;
                case Mode::store_absolute:
                    operand = "& " + hex(word(a), 4) + ", " + value(a + 2);
                    break;
                case Mode::store_indexed:
                    operand = "& " + hex(word(a), 4) + " + " + l + ", " + value(a + 2);
                    break;
                case Mode::reg_immediate:
                    operand = r + ", " + value(a);
                    break;
                case Mode::reg_absolute:
                    operand = "& " + hex(word(a), 4);
                    break;
                case Mode::reg_pc:
                    operand = "* " + offset(bytes[a]);
                    break;
                case Mode::reg_bp:
                    operand = "& bp " + offset(bytes[a]);
                    break;
                default:
                    break;
                }
                if (e.mode == Mode::reg_absolute || e.mode == Mode::reg_pc || e.mode == Mode::reg_bp)
                {
                    if (op == 0x3b || op == 0x1b || op == 0x1d)
                        operand += ", " + r;
                    else
                        operand = r + ", " + operand;
                }
                out += "\t" + std::string(opcode(op).name) + (operand.empty() ? "" : " " + operand) + "\n";
                i += 2 + e.size;
            }
            return out;
        }
    };
} // namespace ANC216
