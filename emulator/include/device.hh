#include <emem.hh>

#pragma once

namespace ANC216
{
    enum DeviceID
    {
        ROM = 0,
        MPME_CARD = 0x0102,
        VIDEO_CARD = 0x0201,
        AUDIO_CARD = 0x0200
    };

    class Device
    {
    protected:
        DeviceID id;
        EmemMapper *emem;

        uint16_t get_addr()
        {
            return emem->where_am_i(this);
        }

    public:

        Device(EmemMapper *emem)
        {
            this->emem = emem;
        }

        virtual void cpu_write(uint16_t value, bool additional_flag) = 0;
        virtual uint16_t cpu_read(uint16_t value, bool additional_flag) = 0;
        
        inline DeviceID cpu_info_req()
        {
            return this->id;
        }
    };
}