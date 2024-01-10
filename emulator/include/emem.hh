#pragma once

#include <common.hh>

class ANC216::EmemMapper
{
private:
    std::pair<uint8_t, Device *> emem[MAX_MEM] = {{0, nullptr}};
    CPU *cpu;

public:
    EmemMapper(const EmuFlags &);
    inline void set_cpu(CPU *);
    uint16_t where_am_i(const Device *);
    void write(uint16_t, uint16_t);
    void read(uint16_t, uint16_t);
    void info_req(uint16_t);
};