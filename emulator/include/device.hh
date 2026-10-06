#pragma once
#include <common.hh>
namespace ANC216
{
    class Device
    {
    protected:
        DeviceID id = ROM;
        EmemMapper *emem;
        EmuFlags flags;
        unsigned transfer_width = 2;
        uint16_t get_addr() const;

    public:
        Device(EmemMapper *, EmuFlags);
        virtual ~Device() = default;
        virtual void cpu_write(uint16_t, bool) = 0;
        virtual uint16_t cpu_read(uint16_t, bool) = 0;
        DeviceID cpu_info_req() const
        {
            return id;
        }
        void set_transfer_width(unsigned width)
        {
            transfer_width = width;
        }
    };
} // namespace ANC216
