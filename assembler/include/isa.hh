#include <map>
#include <string>
#include <vector>
#include <tuple>

#pragma once
#include "../../common/encoding.hh"

namespace ANC216
{
    enum AddressingMode
    {
        IMPLIED_MODE,
        IMMEDIATE_BYTE,
        IMMEDIATE_WORD,
        REGISTER_ACCESS_MODE,
        REGISTER_TO_REGISTER_MODE,
        MEMORY_ABSOULTE,
        MEMORY_ABSOULTE_INDEXED,
        MEMORY_INDIRECT,
        MEMORY_INDIRECT_INDEXED,
        MEMORY_RELATIVE_TO_PC,
        MEMORY_RELATIVE_TO_PC_WITH_REGISTER,
        MEMORY_RELATIVE_TO_BP,
        MEMORY_RELATIVE_TO_BP_WITH_REGISTER,
        IMMEDIATE_TO_MEMORY_ABSOLUTE,
        IMMEDIATE_TO_MEMORY_ABSOLUTE_INDEXED,
        IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP,
        IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP_WITH_REGISTER,
        REGISTER_TO_MEMORY_ABSOULTE,
        MEMORY_ABSOULTE_TO_REGISTER,
        IMMEDIATE_TO_REGISTER,
        REGISTER_TO_MEMORY_RELATIVE_TO_PC,
        MEMORY_RELATIVE_TO_PC_TO_REGISTER,
        REGISTER_TO_MEMORY_RELATIVE_TO_BP,
        MEMORY_RELATIVE_TO_BP_TO_REGISTER,
        LOW_REGISTER_TO_MEMORY_ABSOLUTE,
        MEMORY_ABSOULTE_TO_LOW_REGISTER,
        IMMEDIATE_TO_LOW_REGISTER,
        LOW_REGISTER_TO_MEMORY_RELATIVE_TO_PC,
        MEMORY_RELATIVE_TO_PC_TO_LOW_REGISTER,
        LOW_REGISTER_TO_MEMORY_RELATIVE_TO_BP,
        MEMORY_RELATIVE_TO_BP_TO_LOW_REGISTER,
    };

    enum AddressingModeFamily
    {
        IMPLIED,
        IMMEDIATE,
        IMMEDIATE_TO_MEMORY,
        MEMORY_RELATED,
        REGISTER_ACCESS,
        REGISTER_TO_MEMORY,
        MEMORY_TO_REGISTER,
        REGISTER_TO_REGISTER,
        INDIRECT,
    };

    AddressingModeFamily get_family(AddressingMode mode)
    {
        switch (mode)
        {
        case IMPLIED_MODE:
            return IMPLIED;
        case IMMEDIATE_BYTE:
        case IMMEDIATE_WORD:
            return IMMEDIATE;
        case REGISTER_ACCESS_MODE:
            return REGISTER_ACCESS;
        case REGISTER_TO_REGISTER_MODE:
            return REGISTER_TO_REGISTER;
        case MEMORY_ABSOULTE:
        case MEMORY_ABSOULTE_INDEXED:
        case MEMORY_RELATIVE_TO_PC:
        case MEMORY_RELATIVE_TO_PC_WITH_REGISTER:
        case MEMORY_RELATIVE_TO_BP_WITH_REGISTER:
        case MEMORY_RELATIVE_TO_BP:
            return MEMORY_RELATED;
        case MEMORY_INDIRECT:
        case MEMORY_INDIRECT_INDEXED:
            return INDIRECT;
        case IMMEDIATE_TO_MEMORY_ABSOLUTE:
        case IMMEDIATE_TO_MEMORY_ABSOLUTE_INDEXED:
        case IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP:
        case IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP_WITH_REGISTER:
            return IMMEDIATE_TO_MEMORY;
        case REGISTER_TO_MEMORY_ABSOULTE:
            return REGISTER_TO_MEMORY;
        case MEMORY_ABSOULTE_TO_REGISTER:
            return MEMORY_TO_REGISTER;
        case IMMEDIATE_TO_REGISTER:
            return MEMORY_TO_REGISTER;
        case REGISTER_TO_MEMORY_RELATIVE_TO_PC:
            return REGISTER_TO_MEMORY;
        case MEMORY_RELATIVE_TO_PC_TO_REGISTER:
            return MEMORY_TO_REGISTER;
        case REGISTER_TO_MEMORY_RELATIVE_TO_BP:
            return REGISTER_TO_MEMORY;
        case MEMORY_RELATIVE_TO_BP_TO_REGISTER:
            return MEMORY_TO_REGISTER;
        case LOW_REGISTER_TO_MEMORY_ABSOLUTE:
            return REGISTER_TO_MEMORY;
        case MEMORY_ABSOULTE_TO_LOW_REGISTER:
            return MEMORY_TO_REGISTER;
        case IMMEDIATE_TO_LOW_REGISTER:
            return MEMORY_TO_REGISTER;
        case LOW_REGISTER_TO_MEMORY_RELATIVE_TO_PC:
            return REGISTER_TO_MEMORY;
        case MEMORY_RELATIVE_TO_PC_TO_LOW_REGISTER:
            return MEMORY_TO_REGISTER;
        case LOW_REGISTER_TO_MEMORY_RELATIVE_TO_BP:
            return REGISTER_TO_MEMORY;
        case MEMORY_RELATIVE_TO_BP_TO_LOW_REGISTER:
            return MEMORY_TO_REGISTER;
        }
        return IMPLIED;
    }

    inline std::map<std::string, std::pair<unsigned char, std::vector<AddressingModeFamily>>> isa = []
    {
        std::map<std::string, std::pair<unsigned char, std::vector<AddressingModeFamily>>> result;
        for (unsigned op = 0; op < 256; ++op)
        {
            const auto spec = anc216_isa::opcode(op);
            if (spec.name.empty())
                continue;
            std::vector<AddressingModeFamily> families;
            for (unsigned f = 0; f < 9; ++f)
                if (spec.families & (1u << f))
                    families.push_back(static_cast<AddressingModeFamily>(f));
            result.emplace(std::string(spec.name), std::make_pair(static_cast<unsigned char>(op), families));
        }
        result["jz"] = result["jeq"];
        result["jnz"] = result["jne"];
        return result;
    }();
} // namespace ANC216
