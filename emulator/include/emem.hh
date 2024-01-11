#pragma once

#include <common.hh>
#include <video.hh>

class ANC216::EmemMapper
{
private:
    std::pair<uint8_t, Device *> *emem = new std::pair<uint8_t, Device *>[MAX_MEM];
    CPU *cpu;

public:
    EmemMapper(const EmuFlags &, Video::Window*);
    ~EmemMapper();
    void set_cpu(CPU *);
    uint16_t where_am_i(const Device *);
    void write(uint16_t, uint16_t);
    void read(uint16_t, uint16_t);
    void info_req(uint16_t);
};