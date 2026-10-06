#pragma once
#include <device.hh>
#include <deque>
#include <mutex>

namespace ANC216
{
    // Text/control events stay queued while I is masked. Polling READ consumes
    // one event; otherwise the CPU delivers it through EINR at an instruction boundary.
    class Keyboard : public Device
    {
        std::deque<uint16_t> events;
        std::mutex mutex;
        uint16_t dropped = 0;

    public:
        Keyboard(EmemMapper *mapper, EmuFlags flags) : Device(mapper, flags)
        {
            id = KEYBOARD;
        }
        void input(uint16_t value);
        uint16_t pending();
        void consume();
        void cpu_write(uint16_t, bool) override;
        uint16_t cpu_read(uint16_t, bool) override;
    };
} // namespace ANC216
