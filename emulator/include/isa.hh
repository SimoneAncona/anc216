#pragma once

#include <map>
#include <string>
#include <vector>
#include <tuple>


namespace ANC216
{
    enum AddressingMode
    {
        IMPLIED_MODE,
        IMMEDIATE_BYTE,
        IMMEDIATE_WORD,
        REGISTER_ACCESS_MODE,
        LOW_REGISTER_ACCESS_MODE,
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

    enum Instruction
    {
        KILL = 0x00,
        RESETI = 0x01,
        CPUID = 0x02,
        SYSCALL = 0x03,
        CALL = 0x04,
        RET = 0x05,
        PUSH = 0x06,
        POP = 0x07,
        PHPC = 0x08,
        POPC = 0x09,
        PHSR = 0x0A,
        POSR = 0x0B,
        PHSP = 0x0C,
        POSP = 0x0D,
        PHBP = 0x0E,
        POBP = 0x0F,
        SETI = 0x10,
        SETT = 0x11,
        SETS = 0x12,
        CLRI = 0x13,
        CLRT = 0x14,
        CLRS = 0x15,
        CLRN = 0x16,
        CLRO = 0x17,
        CLRC = 0x18,
        IREQ = 0x19,
        REQ = 0x1A,
        WRITE = 0x1B,
        HREQ = 0x1C,
        HWRITE = 0x1D,
        READ = 0x1E,
        PAREQ = 0x1F,
        CMP = 0x20,
        CAREQ = 0x21,
        JMP = 0x22,
        JEQ = 0x23,
        JZ = 0x23,
        JNE = 0x24,
        JNZ = 0x24,
        JGE = 0x25,
        JGR = 0x26,
        JLE = 0x27,
        JLS = 0x28,
        JO = 0x29,
        JNO = 0x2A,
        JN = 0x2B,
        JNN = 0x2C,
        INC = 0x2D,
        DEC = 0x2E,
        ADD = 0x2F,
        SUB = 0x30,
        NEG = 0x31,
        AND = 0x32,
        OR = 0x33,
        XOR = 0x34,
        NOT = 0x35,
        SIGN = 0x36,
        SHL = 0x37,
        SHR = 0x38,
        PAR = 0x39,
        LOAD = 0x3A,
        STORE = 0x3B,
        TRAN = 0x3C,
        SWAP = 0x3D,
        LDSR = 0x3E,
        LDSP = 0x3F,
        LDBP = 0x40,
        STSR = 0x41,
        STSP = 0x42,
        STBP = 0x43,
        TRSR = 0x44,
        TRSP = 0x45,
        TRBP = 0x46,
        SILI = 0x50,
        SIHI = 0x51,
        SELI = 0x52,
        SEHI = 0x53,
        SBP = 0x54,
        STP = 0x55,
        TILI = 0x56,
        TIHI = 0x57,
        TELI = 0x58,
        TEHI = 0x59,
        TBP = 0x5A,
        TTP = 0x5B,
        LCPID = 0x5C,
        TCPID = 0x5D,
        TIME = 0x60,
        TSTART = 0x61,
        TSTOP = 0x62,
        TRT = 0x63,
    };
}