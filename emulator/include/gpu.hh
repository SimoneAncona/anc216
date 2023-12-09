#include <device.hh>
#include <emem.hh>

#pragma once

namespace ANC216
{
    class AVC64 : public Device
    {
    public:
        AVC64(EmemMapper *emem) : Device(emem)
        {       
            this->id = VIDEO_CARD;    
        }

        void cpu_write(uint16_t value, bool additional_flag)
        {

        }

        uint16_t cpu_read(uint16_t value, bool additional_flag)
        {

        }
    };
}