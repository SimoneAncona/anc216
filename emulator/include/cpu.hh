#pragma once
#include <common.hh>
#include <array>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <map>
#include <optional>

namespace ANC216
{
    struct CPUInfo
    {
        std::array<int16_t, 8> reg{};
        std::array<uint16_t, 6> mtu{};
        uint8_t sr;
        uint16_t sp, bp, pc, current_instruction;
    };
    struct CPUDebugState
    {
        bool running, halted;
        uint64_t instructions, stop_serial;
        std::optional<uint16_t> breakpoint;
        bool temporary;
    };
    class CPU
    {
        std::array<int16_t, 8> reg{};
        uint8_t sr = 0x3c;
        uint16_t sp = 0x3000, bp = 0x3000, pc = ROM_ADDR, current_instruction = 0;
        std::array<uint8_t, MAX_MEM> imem{};
        // Inclusive physical bounds, with absolute user addresses rebased by lower indices.
        std::array<uint16_t, 6> mtu{0, 0xfeff, 0, 0xffff, 0x3200, 0xfeff};
        uint16_t cpuid = 0x8000, timer = 0;
        bool timer_running = false;
        std::chrono::steady_clock::time_point timer_tick;
        EmemMapper *emem;
        EmuFlags flags;
        std::thread worker;
        mutable std::recursive_mutex mutex;
        std::condition_variable_any wake;
        bool killed = false, running = false;
        bool reset_requested = false, shutdown_requested = false;
        std::string failure;
        std::map<uint16_t, bool> breakpoints;
        std::optional<uint16_t> breakpoint_hit, resume_breakpoint, temporary_pc, temporary_sp;
        bool temporary_hit = false;
        uint64_t instruction_count = 0, stop_serial = 0;
        void cycle();
        void execute(bool honor_breakpoints = false);
        void update_timer();
        uint16_t mapped(uint16_t, bool external = false, bool is_ireq = false) const;
        void check(uint16_t, unsigned, bool write = false) const;
        uint16_t read(uint16_t, unsigned = 2) const;
        void write(uint16_t, uint16_t, unsigned = 2);
        void push(uint16_t, unsigned = 2);
        uint16_t pop(unsigned = 2);
        void interrupt(uint16_t vector, int code = -1);
        void nz(uint16_t, unsigned);
        void set_register(unsigned, uint16_t, unsigned);

    public:
        CPU(EmemMapper *, const EmuFlags &);
        ~CPU();
        CPU(const CPU &) = delete;
        CPU &operator=(const CPU &) = delete;
        void load_init_state();
        void load(const std::vector<uint8_t> &, uint16_t address = ROM_ADDR);
        void start();
        void stop();
        void shutdown();
        void wait();
        void step();
        void request_soft_reset();
        void request_shutdown();
        bool einr(uint16_t address = 0, uint16_t data = 0, uint8_t type = 1);
        uint16_t get_pc();
        uint16_t get_current_instruction();
        CPUInfo get_info();
        CPUDebugState debug_state() const;
        void set_breakpoint(uint16_t, bool enabled = true);
        bool remove_breakpoint(uint16_t);
        void clear_breakpoints();
        std::map<uint16_t, bool> list_breakpoints() const;
        void run_until(uint16_t address, std::optional<uint16_t> stack = std::nullopt);
        void debug_set(const std::string &name, uint16_t value);
        std::vector<uint8_t> memory(uint16_t address, size_t size) const;
        uint8_t peek(uint16_t) const;
        void poke(uint16_t, uint8_t);
        bool halted() const;
        std::string error() const;
    };
} // namespace ANC216
