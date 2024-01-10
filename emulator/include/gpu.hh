#pragma once

#include <common.hh>

class ANC216::AVC64 : public ANC216::Device
{
public:
    AVC64(ANC216::EmemMapper *emem);
    void cpu_write(uint16_t value, bool additional_flag);
    uint16_t cpu_read(uint16_t value, bool additional_flag);
};