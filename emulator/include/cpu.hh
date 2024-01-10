#pragma once

#include <common.hh>
#include <isa.hh>

class ANC216::CPU
{
private:
    int16_t reg[8] = {0};

    uint8_t sr;
    uint16_t sp;
    uint16_t bp;

    uint16_t pc;
    uint16_t current_instruction;

    AddressingMode current_addressing_mode;
    uint8_t argsize;

    uint8_t imem[MAX_MEM] = {0};
    EmemMapper *emem;

    bool killed = false;
    bool running;

    inline int8_t get_lower(int16_t val);
    inline void fetch_instruction();
    inline void get_addressing_mode();
    inline void store_data();
    inline uint16_t fetch_data();
    inline void execute();

public:
    CPU(EmemMapper *mapper, const EmuFlags &flags);
    inline void start();
    inline void stop();
    inline uint16_t get_pc();
    inline uint16_t get_current_instruction();
    inline int16_t *get_registers();
    inline void _cycle();
    inline void einr();
};