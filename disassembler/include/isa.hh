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
        NONE,
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
        case MEMORY_ABSOULTE_TO_REGISTER:
        case IMMEDIATE_TO_REGISTER:
        case REGISTER_TO_MEMORY_RELATIVE_TO_PC:
        case MEMORY_RELATIVE_TO_PC_TO_REGISTER:
        case REGISTER_TO_MEMORY_RELATIVE_TO_BP:
        case MEMORY_RELATIVE_TO_BP_TO_REGISTER:
        case LOW_REGISTER_TO_MEMORY_ABSOLUTE:
        case MEMORY_ABSOULTE_TO_LOW_REGISTER:
        case IMMEDIATE_TO_LOW_REGISTER:
        case LOW_REGISTER_TO_MEMORY_RELATIVE_TO_PC:
        case MEMORY_RELATIVE_TO_PC_TO_LOW_REGISTER:
        case LOW_REGISTER_TO_MEMORY_RELATIVE_TO_BP:
        case MEMORY_RELATIVE_TO_BP_TO_LOW_REGISTER:
            return MEMORY_TO_REGISTER;
        }
        return IMPLIED;
    }

    std::map<unsigned char, std::pair<std::string, std::vector<AddressingModeFamily>>> isa =
        {
            {0x00, {"kill",    {IMPLIED}}},
            {0x01, {"reset",   {IMPLIED}}},
            {0x02, {"cpuid",   {IMPLIED}}},
            {0x03, {"syscall", {IMPLIED}}},
            {0x04, {"call",    {MEMORY_RELATED}}},
            {0x05, {"ret",     {IMPLIED}}},
            {0x06, {"push",    {REGISTER_ACCESS, IMMEDIATE}}},
            {0x07, {"pop",     {REGISTER_ACCESS}}},
            {0x08, {"phpc",    {IMPLIED}}},
            {0x09, {"popc",    {IMPLIED}}},
            {0x0A, {"phsr",    {IMPLIED}}},
            {0x0B, {"posr",    {IMPLIED}}},
            {0x0C, {"phsp",    {IMPLIED}}},
            {0x0D, {"posp",    {IMPLIED}}},
            {0x0E, {"phbp",    {IMPLIED}}},
            {0x0F, {"pobp",    {IMPLIED}}},
            {0x10, {"seti",    {IMPLIED}}},
            {0x11, {"sett",    {IMPLIED}}},
            {0x12, {"sets",    {IMPLIED}}},
            {0x13, {"clri",    {IMPLIED}}},
            {0x14, {"clrt",    {IMPLIED}}},
            {0x15, {"clrs",    {IMPLIED}}},
            {0x16, {"clrn",    {IMPLIED}}},
            {0x17, {"clro",    {IMPLIED}}},
            {0x18, {"clrc",    {IMPLIED}}},
            {0x19, {"ireq",    {REGISTER_ACCESS, MEMORY_RELATED}}},
            {0x1A, {"req",     {REGISTER_ACCESS, MEMORY_RELATED}}},
            {0x1B, {"write",   {MEMORY_TO_REGISTER, IMMEDIATE_TO_MEMORY}}},
            {0x1C, {"hreq",    {REGISTER_ACCESS, MEMORY_RELATED}}},
            {0x1D, {"hwrite",  {MEMORY_TO_REGISTER, IMMEDIATE_TO_MEMORY}}},
            {0x1E, {"read",    {REGISTER_ACCESS, MEMORY_RELATED}}},
            {0x1F, {"pareq",   {IMPLIED}}},
            {0x20, {"cmp",     {REGISTER_TO_REGISTER, MEMORY_TO_REGISTER}}},
            {0x21, {"careq",   {IMPLIED}}},
            {0x22, {"jmp",     {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {0x23, {"jeq",     {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {0x23, {"jz",      {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {0x24, {"jne",     {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {0x24, {"jnz",     {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {0x25, {"jge",     {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {0x26, {"jgr",     {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {0x27, {"jle",     {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {0x28, {"jls",     {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {0x29, {"jo",      {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {0x2A, {"jno",     {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {0x2B, {"jn",      {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {0x2C, {"jnn",     {MEMORY_RELATED, IMMEDIATE, INDIRECT}}},
            {0x2D, {"inc",     {REGISTER_ACCESS}}},
            {0x2E, {"dec",     {REGISTER_ACCESS}}},
            {0x2F, {"add",     {REGISTER_TO_REGISTER, MEMORY_TO_REGISTER}}},
            {0x30, {"sub",     {REGISTER_TO_REGISTER, MEMORY_TO_REGISTER}}},
            {0x31, {"neg",     {REGISTER_ACCESS}}},
            {0x32, {"and",     {REGISTER_TO_REGISTER, MEMORY_TO_REGISTER}}},
            {0x33, {"or",      {REGISTER_TO_REGISTER, MEMORY_TO_REGISTER}}},
            {0x34, {"xor",     {REGISTER_TO_REGISTER, MEMORY_TO_REGISTER}}},
            {0x35, {"not",     {REGISTER_ACCESS}}},
            {0x36, {"sign",    {REGISTER_ACCESS, MEMORY_RELATED}}},
            {0x37, {"shl",     {REGISTER_TO_REGISTER, MEMORY_TO_REGISTER}}},
            {0x38, {"shr",     {REGISTER_TO_REGISTER, MEMORY_TO_REGISTER}}},
            {0x39, {"par",     {REGISTER_ACCESS, MEMORY_RELATED}}},
            {0x3A, {"load",    {MEMORY_TO_REGISTER}}},
            {0x3B, {"store",   {MEMORY_TO_REGISTER, IMMEDIATE_TO_MEMORY}}},
            {0x3C, {"tran",    {REGISTER_TO_REGISTER}}},
            {0x3D, {"swap",    {REGISTER_TO_REGISTER}}},
            {0x3E, {"ldsr",    {REGISTER_ACCESS, MEMORY_RELATED, IMMEDIATE}}},
            {0x3F, {"ldsp",    {REGISTER_ACCESS, MEMORY_RELATED, IMMEDIATE}}},
            {0x40, {"ldbp",    {REGISTER_ACCESS, MEMORY_RELATED, IMMEDIATE}}},
            {0x41, {"stsr",    {MEMORY_RELATED}}},
            {0x42, {"stsp",    {MEMORY_RELATED}}},
            {0x43, {"stbp",    {MEMORY_RELATED}}},
            {0x44, {"trsr",    {REGISTER_ACCESS}}},
            {0x45, {"trsp",    {REGISTER_ACCESS}}},
            {0x46, {"trbp",    {REGISTER_ACCESS}}},
            {0x50, {"sili",    {REGISTER_ACCESS, IMMEDIATE, MEMORY_RELATED}}},
            {0x51, {"sihi",    {REGISTER_ACCESS, IMMEDIATE, MEMORY_RELATED}}},
            {0x52, {"seli",    {REGISTER_ACCESS, IMMEDIATE, MEMORY_RELATED}}},
            {0x53, {"sehi",    {REGISTER_ACCESS, IMMEDIATE, MEMORY_RELATED}}},
            {0x54, {"sbp",     {REGISTER_ACCESS, IMMEDIATE, MEMORY_RELATED}}},
            {0x55, {"stp",     {REGISTER_ACCESS, IMMEDIATE, MEMORY_RELATED}}},
            {0x56, {"tili",    {REGISTER_ACCESS}}},
            {0x57, {"tihi",    {REGISTER_ACCESS}}},
            {0x58, {"teli",    {REGISTER_ACCESS}}},
            {0x59, {"tehi",    {REGISTER_ACCESS}}},
            {0x5A, {"tbp",     {REGISTER_ACCESS}}},
            {0x5B, {"ttp",     {REGISTER_ACCESS}}},
            {0x5C, {"lcpid",   {REGISTER_ACCESS, IMMEDIATE, MEMORY_RELATED}}},
            {0x5D, {"tcpid",   {REGISTER_ACCESS}}},
            {0x60, {"time",    {MEMORY_RELATED, REGISTER_ACCESS, IMMEDIATE}}},
            {0x61, {"tstart",  {IMPLIED}}},
            {0x62, {"tstop",   {IMPLIED}}},
            {0x63, {"trt",     {REGISTER_ACCESS}}},
        };
}