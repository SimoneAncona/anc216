#pragma once
#include <csignal>
#include <cpu.hh>

namespace ANC216
{
    // Signal handlers cannot take CPU locks. Defer the shutdown pin to the
    // main/debug event loop, which runs between instructions or while paused.
    inline volatile std::sig_atomic_t host_shutdown_requested = 0;
    inline void handle_host_interrupt(int)
    {
        host_shutdown_requested = 1;
    }
    inline bool poll_host_interrupt(CPU &cpu)
    {
        if (host_shutdown_requested)
        {
            host_shutdown_requested = 0;
            cpu.request_shutdown();
            return true;
        }
        return false;
    }
} // namespace ANC216
