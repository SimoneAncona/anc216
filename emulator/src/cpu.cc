#include <cpu.hh>

#pragma once

#define X1_REG_MASK 0b11'000'111
#define X2_REG_MASK 0b11'000'000

#define X1_GET_REG(x) (x & 0b00111000) >> 3
#define X2_GET_REG(x) x & 0b00000111

inline int8_t ANC216::CPU::get_lower(int16_t val)
{
    return val & 0xFF;
}

inline void ANC216::CPU::fetch_instruction()
{
    current_instruction = imem[pc += 2];
    get_addressing_mode();
}

inline void ANC216::CPU::get_addressing_mode()
{
    uint8_t addr = current_instruction >> 8;
    if (addr == 0)
    {
        current_addressing_mode = IMPLIED_MODE;
    }
    else if (addr == 0b00'001'000)
    {
        current_addressing_mode = IMMEDIATE_BYTE;
        argsize = 1;
    }
    else if (addr == 0b00'010'000)
    {
        current_addressing_mode = IMMEDIATE_WORD;
        argsize = 2;
    }
    else if ((addr & 0b11'000'111) == 0b00'000'001)
    {
        current_addressing_mode = REGISTER_ACCESS_MODE;
        argsize = 2;
    }
    else if ((addr & 0b11'000'111) == 0b00'000'010)
    {
        current_addressing_mode = REGISTER_ACCESS_MODE;
        argsize = 1;
    }
    else if ((addr & 0b11'000'000) == 0b01'000'000)
    {
        current_addressing_mode = REGISTER_TO_REGISTER_MODE;
    }
    else if (addr == 0b10'000'000)
    {
        current_addressing_mode = MEMORY_ABSOULTE;
        argsize = 2;
    }
    else if ((addr & 0b11'000'111) == 0b10'000'001)
    {
        current_addressing_mode = MEMORY_ABSOULTE_INDEXED;
        argsize = 2;
    }
    else if (addr == 0b10'000'100)
    {
        current_addressing_mode = MEMORY_RELATIVE_TO_PC;
        argsize = 1;
    }
    else if (addr == 0b10'001'100)
    {
        current_addressing_mode = MEMORY_RELATIVE_TO_BP;
        argsize = 1;
    }
    else if ((addr & 0b11'000'111) == 0b10'000'101)
    {
        current_addressing_mode = MEMORY_RELATIVE_TO_PC_WITH_REGISTER;
    }
    else if ((addr & 0b11'000'111) == 0b10'000'110)
    {
        current_addressing_mode = MEMORY_RELATIVE_TO_BP_WITH_REGISTER;
    }
    else if (addr == 0b10'000'010)
    {
        current_addressing_mode = MEMORY_INDIRECT;
        argsize = 2;
    }
    else if ((addr & 0b11'000'111) == 0b10'000'011)
    {
        current_addressing_mode = MEMORY_INDIRECT_INDEXED;
        argsize = 2;
    }
    else if (addr == 0b11)
    {
        current_addressing_mode = IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP;
        argsize = 1;
    }
    else if (addr == 0b1011)
    {
        current_addressing_mode = IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP;
        argsize = 2;
    }
    else if ((addr & 0b11'000'111) == 0b00'000'100)
    {
        current_addressing_mode = IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP_WITH_REGISTER;
        argsize = 1;
    }
    else if ((addr & 0b11'000'111) == 0b00'000'101)
    {
        current_addressing_mode = IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP_WITH_REGISTER;
        argsize = 2;
    }
    else if (addr == 0b110)
    {
        current_addressing_mode = IMMEDIATE_TO_MEMORY_ABSOLUTE;
        argsize = 3;
    }
    else if (addr == 0b1110)
    {
        current_addressing_mode = IMMEDIATE_TO_MEMORY_ABSOLUTE;
        argsize = 4;
    }
    else if ((addr & 0b11'000'111) == 0b111)
    {
        current_addressing_mode = IMMEDIATE_TO_MEMORY_ABSOLUTE_INDEXED;
        argsize = 1;
    }
    else if ((addr & X1_REG_MASK) == 0b11'000'000)
    {
        current_addressing_mode = MEMORY_ABSOULTE_TO_REGISTER;
    }
    else if ((addr & X1_REG_MASK) == 0b11'000'001)
    {
        current_addressing_mode = IMMEDIATE_TO_REGISTER;
    }
    else if ((addr & X1_REG_MASK) == 0b11'000'010)
    {
        current_addressing_mode = MEMORY_RELATIVE_TO_PC_TO_REGISTER;
    }
    else if ((addr & X1_REG_MASK) == 0b11'000'011)
    {
        current_addressing_mode = MEMORY_RELATIVE_TO_BP_TO_REGISTER;
    }
    else if ((addr & X1_REG_MASK) == 0b11'000'100)
    {
        current_addressing_mode = MEMORY_ABSOULTE_TO_LOW_REGISTER;
    }
    else if ((addr & X1_REG_MASK) == 0b11'000'101)
    {
        current_addressing_mode = IMMEDIATE_TO_LOW_REGISTER;
    }
    else if ((addr & X1_REG_MASK) == 0b11'000'110)
    {
        current_addressing_mode = MEMORY_RELATIVE_TO_PC_TO_LOW_REGISTER;
    }
    else if ((addr & X1_REG_MASK) == 0b11'000'111)
        current_addressing_mode = MEMORY_RELATIVE_TO_BP_TO_LOW_REGISTER;
}

inline void ANC216::CPU::store_data()
{
}

inline uint16_t ANC216::CPU::fetch_data()
{
    switch (current_addressing_mode)
    {
    case IMMEDIATE_BYTE:
        return imem[pc++];
    case IMMEDIATE_WORD:
        return imem[pc++] << 8 | imem[pc++];
    case REGISTER_ACCESS_MODE:
        return reg[X1_GET_REG(current_addressing_mode)];
    case MEMORY_ABSOULTE:
    case MEMORY_INDIRECT:
        return imem[imem[pc++] << 8 | imem[pc++]];
    case MEMORY_ABSOULTE_INDEXED:
    case MEMORY_INDIRECT_INDEXED:
        return imem[(imem[pc++] << 8 | imem[pc++]) + (int8_t)reg[X1_GET_REG(current_addressing_mode)]];
    case MEMORY_RELATIVE_TO_PC:
        return imem[pc + (int8_t)imem[pc++]];
    case MEMORY_RELATIVE_TO_PC_WITH_REGISTER:
        return imem[pc + (int8_t)reg[X1_GET_REG(current_addressing_mode)]];
    case MEMORY_RELATIVE_TO_BP:
        return imem[bp + (int8_t)imem[pc++]];
    case MEMORY_RELATIVE_TO_BP_WITH_REGISTER:
        return imem[bp + (int8_t)reg[X1_GET_REG(current_addressing_mode)]];
    case IMMEDIATE_TO_MEMORY_ABSOLUTE:
        pc += 2;
        if (argsize == 4)
            return imem[pc++] << 8 | imem[pc++];
        return imem[pc++];
    case IMMEDIATE_TO_MEMORY_ABSOLUTE_INDEXED:
        pc += 2;
        if (argsize == 4)
            return imem[pc++] << 8 | imem[pc++];
        return imem[pc++];
    case IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP:
        pc += 1;
        if (argsize == 3)
            return imem[pc++] << 8 | imem[pc++];
        return imem[pc++];
    case IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP_WITH_REGISTER:
        if (argsize == 2)
            return imem[pc++] << 8 | imem[pc++];
        return imem[pc++];
    case REGISTER_TO_MEMORY_ABSOULTE:
        pc += 2;
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
    case NONE:
    default:
        return 0;
    }
}

inline void ANC216::CPU::execute()
{
    uint8_t ins = current_instruction & 0xFF;
    switch (ins)
    {
    case KILL:
    case RESETI:
    case CPUID:
    case SYSCALL:
    case CALL:
    case RET:
    case PUSH:
    case POP:
    case PHPC:
    case POPC:
    case PHSR:
    case POSR:
    case PHSP:
    case POSP:
    case PHBP:
    case POBP:
    case SETI:
    case SETT:
    case SETS:
    case CLRI:
    case CLRT:
    case CLRS:
    case CLRN:
    case CLRO:
    case CLRC:
    case IREQ:
    case REQ:
    case WRITE:
    case HREQ:
    case HWRITE:
    case READ:
    case PAREQ:
    case CMP:
    case CAREQ:
    case JMP:
    case JEQ:
    case JNE:
    case JGE:
    case JGR:
    case JLE:
    case JLS:
    case JO:
    case JNO:
    case JN:
    case JNN:
    case INC:
    case DEC:
    case ADD:
    case SUB:
    case NEG:
    case AND:
    case OR:
    case XOR:
    case NOT:
    case SIGN:
    case SHL:
    case SHR:
    case PAR:
    case LOAD:
    case STORE:
    case TRAN:
    case SWAP:
    case LDSR:
    case LDSP:
    case LDBP:
    case STSR:
    case STSP:
    case STBP:
    case TRSR:
    case TRSP:
    case TRBP:
    case SILI:
    case SIHI:
    case SELI:
    case SEHI:
    case SBP:
    case STP:
    case TILI:
    case TIHI:
    case TELI:
    case TEHI:
    case TBP:
    case TTP:
    case LCPID:
    case TCPID:
    case TIME:
    case TSTART:
    case TSTOP:
    case TRT:
        return;
    }
}

ANC216::CPU::CPU(EmemMapper *mapper, const EmuFlags &flags)
{
    this->emem = mapper;
    if (flags.debug_mode)
        running = false;
    else
        running = true;
    pc = ROM_ADDR;
    std::thread thread([this]
                       { this->_cycle(); });
    thread.join();
}

inline void ANC216::CPU::start()
{
    running = true;
}

inline void ANC216::CPU::stop()
{
    running = false;
}

inline uint16_t ANC216::CPU::get_pc()
{
    return pc;
}

inline uint16_t ANC216::CPU::get_current_instruction()
{
    return current_instruction;
}

inline int16_t *ANC216::CPU::get_registers()
{
    return reg;
}

inline void ANC216::CPU::_cycle()
{
    while (!killed)
    {
        if (running)
        {
            fetch_instruction();
            execute();
        }
    }
}

inline void ANC216::CPU::einr()
{
}
