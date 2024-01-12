#pragma once

#include <common.hh>
#include <isa.hh>
#include <thread>

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
    uint16_t arg;

    uint8_t *imem = new uint8_t[MAX_MEM];
    EmemMapper *emem;
    std::thread *thread;

    bool killed = false;
    bool running;

    inline int8_t get_lower(int16_t);
    inline void fetch_instruction();
    inline void get_addressing_mode();
    inline void store_data();
    inline void load_reg(uint16_t);
    inline std::pair<uint8_t, uint16_t> fetch_data();
    inline void execute();
    inline void nmi();

public:
    CPU(EmemMapper*, const EmuFlags&);
    ~CPU();
    inline void load_init_state();
    inline void start();
    inline void stop();
    inline uint16_t get_pc();
    inline uint16_t get_current_instruction();
    inline int16_t *get_registers();
    inline void _cycle();
    inline void einr();
    void wait();
};