#pragma once

#include <common.hh>

class ANC216::Device
{
protected:
    DeviceID id;
    EmemMapper *emem;

    inline uint16_t get_addr() const;

public:
    Device(EmemMapper *emem);

    virtual void cpu_write(uint16_t value, bool additional_flag) = 0;
    virtual uint16_t cpu_read(uint16_t value, bool additional_flag) = 0;

    inline DeviceID cpu_info_req();
};