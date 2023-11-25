#include <map>
#include <string>
#include <vector>
#include <tuple>

#pragma once

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

    std::map<std::string, std::pair<unsigned char, std::vector<AddressingModeFamily>>> isa =
        {
            {"kill",    {0x00, {IMPLIED}}},
            {"reset",   {0x01, {IMPLIED}}},
            {"cpuid",   {0x02, {IMPLIED}}},
            {"syscall", {0x03, {IMPLIED}}},
            {"call",    {0x04, {MEMORY_RELATED}}},
            {"ret",     {0x05, {IMPLIED}}},
            {"push",    {0x06, {REGISTER_ACCESS, IMMEDIATE}}},
            {"pop",     {0x07, {REGISTER_ACCESS}}},
            {"phpc",    {0x08, {IMPLIED}}},
            {"popc",    {0x09, {IMPLIED}}},
            {"phsr",    {0x0A, {IMPLIED}}},
            {"posr",    {0x0B, {IMPLIED}}},
            {"phsp",    {0x0C, {IMPLIED}}},
            {"posp",    {0x0D, {IMPLIED}}},
            {"phbp",    {0x0E, {IMPLIED}}},
            {"pobp",    {0x0F, {IMPLIED}}},
            {"seti",    {0x10, {IMPLIED}}},
            {"sett",    {0x11, {IMPLIED}}},
            {"sets",    {0x12, {IMPLIED}}},
            {"clri",    {0x13, {IMPLIED}}},
            {"clrt",    {0x14, {IMPLIED}}},
            {"clrs",    {0x15, {IMPLIED}}},
            {"clrn",    {0x16, {IMPLIED}}},
            {"clro",    {0x17, {IMPLIED}}},
            {"clrc",    {0x18, {IMPLIED}}},
            {"ireq",    {0x19, {REGISTER_ACCESS, MEMORY_RELATED}}},
            {"req",     {0x1A, {REGISTER_ACCESS, MEMORY_RELATED}}},
            {"write",   {0x1B, {REGISTER_TO_MEMORY, IMMEDIATE_TO_MEMORY}}},
            {"hreq",    {0x1C, {REGISTER_ACCESS, MEMORY_RELATED}}},
            {"hwrite",  {0x1D, {REGISTER_TO_MEMORY, IMMEDIATE_TO_MEMORY}}},
            {"read",    {0x1E, {REGISTER_ACCESS, MEMORY_RELATED}}},
            {"pareq",   {0x1F, {IMPLIED}}},
            {"cmp",     {0x20, {REGISTER_TO_REGISTER, MEMORY_TO_REGISTER}}},
            {"careq",   {0x21, {IMPLIED}}},
            {"jmp",     {0x22, {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {"jeq",     {0x23, {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {"jz",      {0x23, {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {"jne",     {0x24, {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {"jnz",     {0x24, {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {"jge",     {0x25, {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {"jgr",     {0x26, {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {"jle",     {0x27, {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {"jls",     {0x28, {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {"jo",      {0x29, {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {"jno",     {0x2A, {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {"jn",      {0x2B, {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {"jnn",     {0x2C, {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {"inc",     {0x2D, {REGISTER_ACCESS}}},
            {"dec",     {0x2E, {REGISTER_ACCESS}}},
            {"add",     {0x2F, {REGISTER_TO_REGISTER, MEMORY_TO_REGISTER}}},
            {"sub",     {0x30, {REGISTER_TO_REGISTER, MEMORY_TO_REGISTER}}},
            {"neg",     {0x31, {REGISTER_ACCESS}}},
            {"and",     {0x32, {REGISTER_TO_REGISTER, MEMORY_TO_REGISTER}}},
            {"or",      {0x33, {REGISTER_TO_REGISTER, MEMORY_TO_REGISTER}}},
            {"xor",     {0x34, {REGISTER_TO_REGISTER, MEMORY_TO_REGISTER}}},
            {"not",     {0x35, {REGISTER_ACCESS}}},
            {"sign",    {0x36, {REGISTER_ACCESS, MEMORY_RELATED}}},
            {"shl",     {0x37, {REGISTER_TO_REGISTER, MEMORY_TO_REGISTER}}},
            {"shr",     {0x38, {REGISTER_TO_REGISTER, MEMORY_TO_REGISTER}}},
            {"par",     {0x39, {REGISTER_ACCESS, MEMORY_RELATED}}},
            {"load",    {0x3A, {MEMORY_TO_REGISTER}}},
            {"store",   {0x3B, {REGISTER_TO_MEMORY, IMMEDIATE_TO_MEMORY}}},
            {"tran",    {0x3C, {REGISTER_TO_REGISTER}}},
            {"swap",    {0x3D, {REGISTER_TO_REGISTER}}},
            {"ldsr",    {0x3E, {REGISTER_ACCESS, MEMORY_RELATED}}},
            {"ldsp",    {0x3F, {REGISTER_ACCESS, MEMORY_RELATED}}},
            {"ldbp",    {0x40, {REGISTER_ACCESS, MEMORY_RELATED}}},
            {"stsr",    {0x41, {MEMORY_RELATED}}},
            {"stsp",    {0x42, {MEMORY_RELATED}}},
            {"stbp",    {0x43, {MEMORY_RELATED}}},
            {"trsr",    {0x44, {REGISTER_ACCESS}}},
            {"trsp",    {0x45, {REGISTER_ACCESS}}},
            {"trbp",    {0x46, {REGISTER_ACCESS}}},
            {"sili",    {0x50, {REGISTER_ACCESS, IMMEDIATE, MEMORY_RELATED}}},
            {"sihi",    {0x51, {REGISTER_ACCESS, IMMEDIATE, MEMORY_RELATED}}},
            {"seli",    {0x52, {REGISTER_ACCESS, IMMEDIATE, MEMORY_RELATED}}},
            {"sehi",    {0x53, {REGISTER_ACCESS, IMMEDIATE, MEMORY_RELATED}}},
            {"sbp",     {0x54, {REGISTER_ACCESS, IMMEDIATE, MEMORY_RELATED}}},
            {"stp",     {0x55, {REGISTER_ACCESS, IMMEDIATE, MEMORY_RELATED}}},
            {"tili",    {0x56, {REGISTER_ACCESS}}},
            {"tihi",    {0x57, {REGISTER_ACCESS}}},
            {"teli",    {0x58, {REGISTER_ACCESS}}},
            {"tehi",    {0x59, {REGISTER_ACCESS}}},
            {"tbp",     {0x5A, {REGISTER_ACCESS}}},
            {"ttp",     {0x5B, {REGISTER_ACCESS}}},
            {"lcpid",   {0x5C, {REGISTER_ACCESS, IMMEDIATE, MEMORY_RELATED}}},
            {"tcpid",   {0x5D, {REGISTER_ACCESS}}},
            {"time",    {0x60, {MEMORY_RELATED, REGISTER_ACCESS}}},
            {"tstart",  {0x61, {IMPLIED}}},
            {"tstop",   {0x62, {IMPLIED}}},
            {"trt",     {0x63, {REGISTER_ACCESS}}},
        };
}