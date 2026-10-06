#pragma once
#include <types.hh>
#include <algorithm>
#include <chrono>
#include <cmath>

namespace ANC216
{
    // Amortize host scheduler overhead across a roughly 1ms instruction batch.
    // Keep batches bounded so debugger/control events can acquire the CPU lock.
    class ExecutionPacer
    {
        using Clock = std::chrono::steady_clock;
        Clock::time_point next = Clock::now();
        std::chrono::duration<double> period;
        bool unrestricted;

    public:
        unsigned batch;
        explicit ExecutionPacer(const EmuFlags &flags) : unrestricted(flags.fast_mode || flags.uncapped)
        {
            const double rate = 100.0 * flags.speed;
            batch = unrestricted ? 1024 : unsigned(std::clamp(std::ceil(rate / 1000.0), 1.0, 1024.0));
            period = std::chrono::duration<double>(batch / rate);
        }
        bool uncapped() const
        {
            return unrestricted;
        }
        Clock::time_point deadline()
        {
            next += std::chrono::duration_cast<Clock::duration>(period);
            next = std::max(next, Clock::now());
            return next;
        }
    };
} // namespace ANC216
