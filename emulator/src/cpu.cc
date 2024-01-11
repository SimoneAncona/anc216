#include <cpu.hh>

#pragma once

#define BYTE_S 1
#define WORD_S 2

#define X1_REG_MASK 0b11'000'111
#define X2_REG_MASK 0b11'000'000

#define X1_GET_REG(x) (x & 0b00111000) >> 3
#define X2_GET_REG(x) x & 0b00000111

#define NEGATIVE_FLAG 0b1000'0000
#define OVERFLOW_FLAG 0b0100'0000
#define INTERRPUTS_FLAG 0b0010'0000
#define TIMER_INTERRUPT_FLAG 0b0001'0000
#define SYSTEM_PRIVILEGES_FLAG 0b0000'1000
#define ZERO_FLAG 0b0000'0010
#define CARRY_FLAG 0b0000'0001

#define CHECK_SYSTEM_PRIVILEGES()\
    if (!(sr & SYSTEM_PRIVILEGES_FLAG))\
    {\
        reg[0] = 4;\
        nmi();\
    }

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
        argsize = 0;
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
        current_addressing_mode = LOW_REGISTER_ACCESS_MODE;
        argsize = 1;
    }
    else if ((addr & 0b11'000'000) == 0b01'000'000)
    {
        current_addressing_mode = REGISTER_TO_REGISTER_MODE;
        argsize = 0;
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
        argsize = 0;
    }
    else if ((addr & 0b11'000'111) == 0b10'000'110)
    {
        current_addressing_mode = MEMORY_RELATIVE_TO_BP_WITH_REGISTER;
        argsize = 0;
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
        argsize = 2;
    }
    else if ((addr & X1_REG_MASK) == 0b11'000'001)
    {
        current_addressing_mode = IMMEDIATE_TO_REGISTER;
        argsize = 2;
    }
    else if ((addr & X1_REG_MASK) == 0b11'000'010)
    {
        current_addressing_mode = MEMORY_RELATIVE_TO_PC_TO_REGISTER;
        argsize = 1;
    }
    else if ((addr & X1_REG_MASK) == 0b11'000'011)
    {
        current_addressing_mode = MEMORY_RELATIVE_TO_BP_TO_REGISTER;
        argsize = 1;
    }
    else if ((addr & X1_REG_MASK) == 0b11'000'100)
    {
        current_addressing_mode = MEMORY_ABSOULTE_TO_LOW_REGISTER;
        argsize = 2;
    }
    else if ((addr & X1_REG_MASK) == 0b11'000'101)
    {
        current_addressing_mode = IMMEDIATE_TO_LOW_REGISTER;
        argsize = 1;
    }
    else if ((addr & X1_REG_MASK) == 0b11'000'110)
    {
        current_addressing_mode = MEMORY_RELATIVE_TO_PC_TO_LOW_REGISTER;
        argsize = 1;
    }
    else if ((addr & X1_REG_MASK) == 0b11'000'111)
    {
        current_addressing_mode = MEMORY_RELATIVE_TO_BP_TO_LOW_REGISTER;
        argsize = 1;
    }

    if (argsize == 1)
        arg = imem[pc + 1];
    if (argsize == 2)
        arg = imem[pc + 1] << 8 | imem[pc + 2];
}

inline void ANC216::CPU::store_data()
{
}

inline void ANC216::CPU::load_reg(uint16_t val)
{
    reg[X1_GET_REG((current_instruction >> 8))] = val;
}
//              <bytes  , value   >
inline std::pair<uint8_t, uint16_t> ANC216::CPU::fetch_data()
{
    switch (current_addressing_mode)
    {
    case IMMEDIATE_BYTE:
        return {1, imem[pc++]};
    case IMMEDIATE_WORD:
        return {2, imem[pc++] << 8 | imem[pc++]};
    case REGISTER_ACCESS_MODE:
        return {2, reg[X1_GET_REG(current_addressing_mode)]};
    case MEMORY_ABSOULTE:
    case MEMORY_INDIRECT:
        return {2, imem[imem[pc++] << 8 | imem[pc++]]};
    case MEMORY_ABSOULTE_INDEXED:
    case MEMORY_INDIRECT_INDEXED:
        return {2, imem[(imem[pc++] << 8 | imem[pc++]) + (int8_t)reg[X1_GET_REG(current_addressing_mode)]]};
    case MEMORY_RELATIVE_TO_PC:
        return {1, imem[pc + (int8_t)imem[pc++]]};
    case MEMORY_RELATIVE_TO_PC_WITH_REGISTER:
        return {1, imem[pc + (int8_t)reg[X1_GET_REG(current_addressing_mode)]]};
    case MEMORY_RELATIVE_TO_BP:
        return {1, imem[bp + (int8_t)imem[pc++]]};
    case MEMORY_RELATIVE_TO_BP_WITH_REGISTER:
        return {1, imem[bp + (int8_t)reg[X1_GET_REG(current_addressing_mode)]]};
    case IMMEDIATE_TO_MEMORY_ABSOLUTE:
        pc += 2;
        if (argsize == 4)
            return {2, imem[pc++] << 8 | imem[pc++]};
        return {1, imem[pc++]};
    case IMMEDIATE_TO_MEMORY_ABSOLUTE_INDEXED:
        pc += 2;
        if (argsize == 4)
            return {2, imem[pc++] << 8 | imem[pc++]};
        return {1, imem[pc++]};
    case IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP:
        pc += 1;
        if (argsize == 3)
            return {2, imem[pc++] << 8 | imem[pc++]};
        return {1, imem[pc++]};
    case IMMEDIATE_TO_MEMORY_RELATIVE_TO_BP_WITH_REGISTER:
        if (argsize == 2)
            return {2, imem[pc++] << 8 | imem[pc++]};
        return {1, imem[pc++]};
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
        return {1, 0};
    }
}

inline void ANC216::CPU::nmi()
{

}

inline void ANC216::CPU::execute()
{
    uint8_t ins = current_instruction & 0xFF;
    std::pair<uint8_t, uint16_t> data = fetch_data();
    switch (ins)
    {
    case KILL:
        killed = true;
        break;
    case RESETI:
        load_init_state();
        break;
    case CPUID:
        reg[0] = (int16_t)0x8000;
        break;
    case SYSCALL:

    case CALL:

    case RET:
        sr = imem[sp - 1];
        pc = imem[sp - 2] << 8 | imem[sp - 3];
        sp = bp;
        break;
    case PUSH:
        if (data.first == BYTE_S)
        {
            imem[sp] = static_cast<uint8_t>(data.second);
            sp++;
        }
        else
        {
            imem[sp] = data.second >> 8;
            imem[sp + (uint16_t)1] = static_cast<uint8_t>(data.second);
            sp += 2;
        }
    case POP:
        if (current_addressing_mode == LOW_REGISTER_ACCESS_MODE)
        {
            sp--;
            load_reg(imem[sp]);
        }
        else
        {
            sp -= 2;
            load_reg(imem[sp] << 8 | imem[sp + 1]);
        }
        break;
    case PHPC:
        imem[sp] = pc >> 8;
        imem[sp + (uint16_t)1] = static_cast<uint8_t>(pc);
        sp += 2;
        break;
    case POPC:
        pc = imem[sp - 1] << 8 | imem[sp - 2];
        sp -= 2;
        break;
    case PHSR:
        imem[sp] = sr;
        sp++;
        break;
    case POSR:
        sr = imem[sp - 1];
        sp--;
        break;
    case PHSP:
        imem[sp] = sp >> 8;
        imem[sp + (uint16_t)1] = static_cast<uint8_t>(sp);
        sp += 2;
        break;
    case POSP:
        sp = imem[sp - 1] << 8 | imem[sp - 2];
        break;
    case PHBP:
        imem[sp] = bp >> 8;
        imem[sp + (uint16_t)1] = static_cast<uint8_t>(bp);
        sp += 2;
        break;
    case POBP:
        bp = imem[sp - 1] << 8 | imem[sp - 2];
        sp -= 2;
        break;
    case SETI:
        CHECK_SYSTEM_PRIVILEGES();
        sr |= INTERRPUTS_FLAG;
        break;
    case SETT:
        CHECK_SYSTEM_PRIVILEGES();
        sr |= TIMER_INTERRUPT_FLAG;
        break;
    case SETS:
        CHECK_SYSTEM_PRIVILEGES();
        sr |= SYSTEM_PRIVILEGES_FLAG;
        break;
    case CLRI:
        CHECK_SYSTEM_PRIVILEGES();
        sr &= ~INTERRPUTS_FLAG;
        break;
    case CLRT:
        CHECK_SYSTEM_PRIVILEGES();
        sr &= ~TIMER_INTERRUPT_FLAG;
        break;
    case CLRS:
        CHECK_SYSTEM_PRIVILEGES();
        sr &= ~SYSTEM_PRIVILEGES_FLAG;
        break;
    case CLRN:
        sr &= ~NEGATIVE_FLAG;
        break;
    case CLRO:
        sr &= ~OVERFLOW_FLAG;
        break;
    case CLRC:
        sr &= ~CARRY_FLAG;
        break;
    case IREQ:
        CHECK_SYSTEM_PRIVILEGES();
        if (current_addressing_mode == REGISTER_ACCESS_MODE)
            emem->info_req(data.second);
        else
            emem->info_req(arg);
        break;
    case REQ:
        CHECK_SYSTEM_PRIVILEGES();
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
    load_init_state();
    std::thread thread([this]
                       { this->_cycle(); });
    thread.join();
}

ANC216::CPU::~CPU()
{
    delete this->imem;
}

inline void ANC216::CPU::load_init_state()
{
    pc = ROM_ADDR;
    sr = 0b00111100;
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
