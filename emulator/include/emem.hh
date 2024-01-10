#pragma once

#include <common.hh>

class ANC216::EmemMapper
{
private:
    std::pair<uint8_t, Device *> emem[MAX_MEM] = {{0, nullptr}};
    CPU *cpu;

public:
    EmemMapper(const EmuFlags &flags);
    inline void set_cpu(CPU *cpu);
    uint16_t where_am_i(const Device *device);
    void write(uint16_t value, uint16_t address);
    void read(uint16_t value, uint16_t address);
};