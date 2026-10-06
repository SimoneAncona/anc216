#pragma once
#include <cstdint>
#include <string_view>

// Canonical wire format used by the emulator and disassembler.
namespace anc216_isa
{
    enum class Mode
    {
        invalid,
        implied,
        immediate,
        reg,
        regs,
        absolute,
        indexed,
        indirect,
        indirect_indexed,
        pc,
        bp,
        pc_reg,
        bp_reg,
        store_bp,
        store_bp_reg,
        store_absolute,
        store_indexed,
        reg_absolute,
        reg_immediate,
        reg_pc,
        reg_bp
    };
    struct Encoding
    {
        Mode mode = Mode::invalid;
        unsigned size = 0; // bytes following the two-byte instruction
        unsigned width = 2;
        unsigned reg = 0, source = 0;
    };
    inline Encoding decode(uint8_t a)
    {
        Encoding e;
        e.reg = (a >> 3) & 7;
        e.source = a & 7;
        if (a == 0)
            e.mode = Mode::implied;
        else if (a == 8 || a == 16)
        {
            e.mode = Mode::immediate;
            e.size = e.width = a == 8 ? 1 : 2;
        }
        else if ((a & 0xc7) == 1 || (a & 0xc7) == 2)
        {
            e.mode = Mode::reg;
            e.width = (a & 7) == 2 ? 1 : 2;
        }
        else if ((a & 0xc0) == 0x40)
            e.mode = Mode::regs;
        else if (a == 0x80)
        {
            e.mode = Mode::absolute;
            e.size = 2;
        }
        else if ((a & 0xc7) == 0x81)
        {
            e.mode = Mode::indexed;
            e.size = 2;
        }
        else if (a == 0x82)
        {
            e.mode = Mode::indirect;
            e.size = 2;
        }
        else if ((a & 0xc7) == 0x83)
        {
            e.mode = Mode::indirect_indexed;
            e.size = 2;
        }
        else if (a == 0x84 || a == 0x8c)
        {
            e.mode = a == 0x84 ? Mode::pc : Mode::bp;
            e.size = 1;
        }
        else if ((a & 0xc7) == 0x85)
            e.mode = Mode::pc_reg;
        else if ((a & 0xc7) == 0x86)
            e.mode = Mode::bp_reg;
        else if (a == 3 || a == 11)
        {
            e.mode = Mode::store_bp;
            e.width = a == 3 ? 1 : 2;
            e.size = 1 + e.width;
        }
        else if ((a & 0xc7) == 4 || (a & 0xc7) == 5)
        {
            e.mode = Mode::store_bp_reg;
            e.size = e.width = (a & 7) == 4 ? 1 : 2;
        }
        else if (a == 6 || a == 14)
        {
            e.mode = Mode::store_absolute;
            e.width = a == 6 ? 1 : 2;
            e.size = 2 + e.width;
        }
        else if ((a & 0xc7) == 7)
        {
            e.mode = Mode::store_indexed;
            e.width = 1;
            e.size = 3;
        }
        else if ((a & 0xc0) == 0xc0)
        {
            e.width = (a & 4) ? 1 : 2;
            switch (a & 3)
            {
            case 0:
                e.mode = Mode::reg_absolute;
                e.size = 2;
                break;
            case 1:
                e.mode = Mode::reg_immediate;
                e.size = e.width;
                break;
            case 2:
                e.mode = Mode::reg_pc;
                e.size = 1;
                break;
            case 3:
                e.mode = Mode::reg_bp;
                e.size = 1;
                break;
            }
        }
        return e;
    }
    enum Family
    {
        IMPLIED,
        IMMEDIATE,
        IMMEDIATE_TO_MEMORY,
        MEMORY_RELATED,
        REGISTER_ACCESS,
        REGISTER_TO_MEMORY,
        MEMORY_TO_REGISTER,
        REGISTER_TO_REGISTER,
        INDIRECT
    };
    struct Opcode
    {
        std::string_view name;
        unsigned families;
        bool privileged;
    };
    inline Opcode opcode(uint8_t op)
    {
        switch (op)
        {
        case 0x00:
            return {"kill", (1u << IMPLIED), true};
        case 0x01:
            return {"reset", (1u << IMPLIED), true};
        case 0x02:
            return {"cpuid", (1u << IMPLIED), false};
        case 0x03:
            return {"syscall", (1u << IMPLIED), false};
        case 0x04:
            return {"call", (1u << MEMORY_RELATED) | (1u << IMMEDIATE) | (1u << INDIRECT), false};
        case 0x05:
            return {"ret", (1u << IMPLIED), false};
        case 0x06:
            return {"push", (1u << REGISTER_ACCESS) | (1u << IMMEDIATE), false};
        case 0x07:
            return {"pop", (1u << REGISTER_ACCESS), false};
        case 0x08:
            return {"phpc", (1u << IMPLIED), false};
        case 0x09:
            return {"popc", (1u << IMPLIED), false};
        case 0x0A:
            return {"phsr", (1u << IMPLIED), false};
        case 0x0B:
            return {"posr", (1u << IMPLIED), false};
        case 0x0C:
            return {"phsp", (1u << IMPLIED), false};
        case 0x0D:
            return {"posp", (1u << IMPLIED), false};
        case 0x0E:
            return {"phbp", (1u << IMPLIED), false};
        case 0x0F:
            return {"pobp", (1u << IMPLIED), false};
        case 0x10:
            return {"seti", (1u << IMPLIED), true};
        case 0x11:
            return {"sett", (1u << IMPLIED), true};
        case 0x12:
            return {"sets", (1u << IMPLIED), true};
        case 0x13:
            return {"clri", (1u << IMPLIED), true};
        case 0x14:
            return {"clrt", (1u << IMPLIED), true};
        case 0x15:
            return {"clrs", (1u << IMPLIED), true};
        case 0x16:
            return {"clrn", (1u << IMPLIED), false};
        case 0x17:
            return {"clro", (1u << IMPLIED), false};
        case 0x18:
            return {"clrc", (1u << IMPLIED), false};
        case 0x19:
            return {"ireq", (1u << REGISTER_ACCESS) | (1u << MEMORY_RELATED), true};
        case 0x1A:
            return {"req", (1u << REGISTER_ACCESS) | (1u << MEMORY_RELATED), true};
        case 0x1B:
            return {"write", (1u << REGISTER_TO_MEMORY) | (1u << IMMEDIATE_TO_MEMORY), true};
        case 0x1C:
            return {"hreq", (1u << REGISTER_ACCESS) | (1u << MEMORY_RELATED), true};
        case 0x1D:
            return {"hwrite", (1u << REGISTER_TO_MEMORY) | (1u << IMMEDIATE_TO_MEMORY), true};
        case 0x1E:
            return {"read", (1u << REGISTER_ACCESS) | (1u << MEMORY_RELATED), true};
        case 0x1F:
            return {"pareq", (1u << IMPLIED), true};
        case 0x20:
            return {"cmp", (1u << REGISTER_TO_REGISTER) | (1u << MEMORY_TO_REGISTER), false};
        case 0x21:
            return {"careq", (1u << IMPLIED), true};
        case 0x22:
            return {"jmp", (1u << MEMORY_RELATED) | (1u << IMMEDIATE) | (1u << INDIRECT), false};
        case 0x23:
            return {"jeq", (1u << MEMORY_RELATED) | (1u << IMMEDIATE) | (1u << INDIRECT), false};
        case 0x24:
            return {"jne", (1u << MEMORY_RELATED) | (1u << IMMEDIATE) | (1u << INDIRECT), false};
        case 0x25:
            return {"jge", (1u << MEMORY_RELATED) | (1u << IMMEDIATE) | (1u << INDIRECT), false};
        case 0x26:
            return {"jgr", (1u << MEMORY_RELATED) | (1u << IMMEDIATE) | (1u << INDIRECT), false};
        case 0x27:
            return {"jle", (1u << MEMORY_RELATED) | (1u << IMMEDIATE) | (1u << INDIRECT), false};
        case 0x28:
            return {"jls", (1u << MEMORY_RELATED) | (1u << IMMEDIATE) | (1u << INDIRECT), false};
        case 0x29:
            return {"jo", (1u << MEMORY_RELATED) | (1u << IMMEDIATE) | (1u << INDIRECT), false};
        case 0x2A:
            return {"jno", (1u << MEMORY_RELATED) | (1u << IMMEDIATE) | (1u << INDIRECT), false};
        case 0x2B:
            return {"jn", (1u << MEMORY_RELATED) | (1u << IMMEDIATE) | (1u << INDIRECT), false};
        case 0x2C:
            return {"jnn", (1u << MEMORY_RELATED) | (1u << IMMEDIATE) | (1u << INDIRECT), false};
        case 0x2D:
            return {"inc", (1u << REGISTER_ACCESS), false};
        case 0x2E:
            return {"dec", (1u << REGISTER_ACCESS), false};
        case 0x2F:
            return {"add", (1u << REGISTER_TO_REGISTER) | (1u << MEMORY_TO_REGISTER), false};
        case 0x30:
            return {"sub", (1u << REGISTER_TO_REGISTER) | (1u << MEMORY_TO_REGISTER), false};
        case 0x31:
            return {"neg", (1u << REGISTER_ACCESS), false};
        case 0x32:
            return {"and", (1u << REGISTER_TO_REGISTER) | (1u << MEMORY_TO_REGISTER), false};
        case 0x33:
            return {"or", (1u << REGISTER_TO_REGISTER) | (1u << MEMORY_TO_REGISTER), false};
        case 0x34:
            return {"xor", (1u << REGISTER_TO_REGISTER) | (1u << MEMORY_TO_REGISTER), false};
        case 0x35:
            return {"not", (1u << REGISTER_ACCESS), false};
        case 0x36:
            return {"sign", (1u << REGISTER_ACCESS) | (1u << MEMORY_RELATED), false};
        case 0x37:
            return {"shl", (1u << REGISTER_TO_REGISTER) | (1u << MEMORY_TO_REGISTER), false};
        case 0x38:
            return {"shr", (1u << REGISTER_TO_REGISTER) | (1u << MEMORY_TO_REGISTER), false};
        case 0x39:
            return {"par", (1u << REGISTER_ACCESS) | (1u << MEMORY_RELATED), false};
        case 0x3A:
            return {"load", (1u << MEMORY_TO_REGISTER), false};
        case 0x3B:
            return {"store", (1u << REGISTER_TO_MEMORY) | (1u << IMMEDIATE_TO_MEMORY), false};
        case 0x3C:
            return {"tran", (1u << REGISTER_TO_REGISTER), false};
        case 0x3D:
            return {"swap", (1u << REGISTER_TO_REGISTER), false};
        case 0x3E:
            return {"ldsr", (1u << REGISTER_ACCESS) | (1u << MEMORY_RELATED) | (1u << IMMEDIATE), false};
        case 0x3F:
            return {"ldsp", (1u << REGISTER_ACCESS) | (1u << MEMORY_RELATED) | (1u << IMMEDIATE), false};
        case 0x40:
            return {"ldbp", (1u << REGISTER_ACCESS) | (1u << MEMORY_RELATED) | (1u << IMMEDIATE), false};
        case 0x41:
            return {"stsr", (1u << MEMORY_RELATED), false};
        case 0x42:
            return {"stsp", (1u << MEMORY_RELATED), false};
        case 0x43:
            return {"stbp", (1u << MEMORY_RELATED), false};
        case 0x44:
            return {"trsr", (1u << REGISTER_ACCESS), false};
        case 0x45:
            return {"trsp", (1u << REGISTER_ACCESS), false};
        case 0x46:
            return {"trbp", (1u << REGISTER_ACCESS), false};
        case 0x50:
            return {"sili", (1u << REGISTER_ACCESS) | (1u << IMMEDIATE) | (1u << MEMORY_RELATED), true};
        case 0x51:
            return {"sihi", (1u << REGISTER_ACCESS) | (1u << IMMEDIATE) | (1u << MEMORY_RELATED), true};
        case 0x52:
            return {"seli", (1u << REGISTER_ACCESS) | (1u << IMMEDIATE) | (1u << MEMORY_RELATED), true};
        case 0x53:
            return {"sehi", (1u << REGISTER_ACCESS) | (1u << IMMEDIATE) | (1u << MEMORY_RELATED), true};
        case 0x54:
            return {"sbp", (1u << REGISTER_ACCESS) | (1u << IMMEDIATE) | (1u << MEMORY_RELATED), true};
        case 0x55:
            return {"stp", (1u << REGISTER_ACCESS) | (1u << IMMEDIATE) | (1u << MEMORY_RELATED), true};
        case 0x56:
            return {"tili", (1u << REGISTER_ACCESS), false};
        case 0x57:
            return {"tihi", (1u << REGISTER_ACCESS), false};
        case 0x58:
            return {"teli", (1u << REGISTER_ACCESS), false};
        case 0x59:
            return {"tehi", (1u << REGISTER_ACCESS), false};
        case 0x5A:
            return {"tbp", (1u << REGISTER_ACCESS), false};
        case 0x5B:
            return {"ttp", (1u << REGISTER_ACCESS), false};
        case 0x5C:
            return {"lcpid", (1u << REGISTER_ACCESS) | (1u << IMMEDIATE) | (1u << MEMORY_RELATED), true};
        case 0x5D:
            return {"tcpid", (1u << REGISTER_ACCESS), false};
        case 0x60:
            return {"time", (1u << MEMORY_RELATED) | (1u << REGISTER_ACCESS) | (1u << IMMEDIATE), true};
        case 0x61:
            return {"tstart", (1u << IMPLIED), true};
        case 0x62:
            return {"tstop", (1u << IMPLIED), true};
        case 0x63:
            return {"trt", (1u << REGISTER_ACCESS), false};
        default:
            return {"", 0, false};
        }
    }
    inline bool valid(uint8_t op, const Encoding &e)
    {
        unsigned f = 0;
        switch (e.mode)
        {
        case Mode::invalid:
            return false;
        case Mode::implied:
            f = 1u << IMPLIED;
            break;
        case Mode::immediate:
            f = 1u << IMMEDIATE;
            break;
        case Mode::reg:
            f = 1u << REGISTER_ACCESS;
            break;
        case Mode::regs:
            f = 1u << REGISTER_TO_REGISTER;
            break;
        case Mode::indirect:
        case Mode::indirect_indexed:
            f = 1u << INDIRECT;
            break;
        case Mode::store_bp:
        case Mode::store_bp_reg:
        case Mode::store_absolute:
        case Mode::store_indexed:
            f = 1u << IMMEDIATE_TO_MEMORY;
            break;
        case Mode::reg_immediate:
            f = 1u << MEMORY_TO_REGISTER;
            break;
        case Mode::reg_absolute:
        case Mode::reg_pc:
        case Mode::reg_bp:
            f = 1u << ((op == 0x3b || op == 0x1b || op == 0x1d) ? REGISTER_TO_MEMORY : MEMORY_TO_REGISTER);
            break;
        default:
            f = 1u << MEMORY_RELATED;
            break;
        }
        if (!(opcode(op).families & f))
            return false;
        // Assembly jump/call immediates are syntax sugar for absolute addressing.
        if ((op == 4 || (op >= 0x22 && op <= 0x2c)) && e.mode == Mode::immediate)
            return false;
        // CALL has a fixed absolute target and a four-byte instruction length.
        if (op == 4 && e.mode != Mode::absolute)
            return false;
        // Special-register transfers have fixed widths.
        if ((op == 0x44 || (op == 0x3e && e.mode == Mode::reg)) && e.width != 1)
            return false;
        if ((op == 0x45 || op == 0x46 || (op >= 0x56 && op <= 0x5b) || op == 0x5d || op == 0x63) && e.width != 2)
            return false;
        return true;
    }
} // namespace anc216_isa
