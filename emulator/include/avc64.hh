#pragma once

#include <common.hh>
#include <gpu.hh>

class ANC216::AVC64 : public ANC216::VideoCard
{
public:
    AVC64(ANC216::EmemMapper *, EmuFlags);
    void cpu_write(uint16_t, bool);
    uint16_t cpu_read(uint16_t, bool);
};