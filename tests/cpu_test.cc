#include <cpu.hh>
#include <emem.hh>
#include <device.hh>
#include <avc64.hh>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <thread>

using namespace ANC216;
static void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
struct Fixture
{
    EmuFlags flags;
    EmemMapper mapper;
    CPU cpu;
    static EmuFlags options()
    {
        EmuFlags f;
        f.novideo = true;
        f.debug_mode = true;
        return f;
    }
    Fixture() : flags(options()), mapper(flags), cpu(&mapper, flags)
    {
    }
    void program(std::vector<uint8_t> bytes)
    {
        cpu.load_init_state();
        cpu.load(bytes);
    }
    CPUInfo run(unsigned cycles = 100)
    {
        for (unsigned i = 0; i < cycles && !cpu.halted(); ++i)
            cpu.step();
        return cpu.get_info();
    }
};
class Echo : public Device
{
public:
    uint16_t last = 0;
    bool additional = false;
    Echo(EmemMapper *mapper, EmuFlags flags) : Device(mapper, flags)
    {
        id = MPME_CARD;
    }
    void cpu_write(uint16_t value, bool extra) override
    {
        last = value;
        additional = extra;
    }
    uint16_t cpu_read(uint16_t value, bool) override
    {
        return value ^ 0x1234;
    }
};
int main()
{
    try
    {
        Fixture f;
        f.program({0xc1, 0x3a, 0x12, 0x34, 0xc5, 0x3a, 0xab, 0x02, 0x2d, 0, 0});
        auto info = f.run();
        require(uint16_t(info.reg[0]) == 0x12ac, "low register writes must preserve high byte");
        require(info.pc == 0xff0b, "instruction length/PC incorrect");
        f.program({0xc1, 0x3a, 0x12, 0x34, 0xc9, 0x3a, 0x56, 0x78, 0x41, 0x3d, 0, 0});
        info = f.run();
        require(uint16_t(info.reg[0]) == 0x5678 && uint16_t(info.reg[1]) == 0x1234, "register SWAP");
        f.program({0xc0, 0x3d, 0x40, 0});
        info = f.run();
        require(f.cpu.halted() && !f.cpu.error().empty(), "memory SWAP must fault");
        // Exhaustive byte add/sub validates both signed overflow and unsigned carry/borrow.
        for (unsigned a = 0; a < 256; ++a)
            for (unsigned b = 0; b < 256; ++b)
                for (bool subtract : {false, true})
                {
                    f.program({0xc5, 0x3a, uint8_t(a), 0xc5, uint8_t(subtract ? 0x30 : 0x2f), uint8_t(b)});
                    f.cpu.step();
                    f.cpu.step();
                    info = f.cpu.get_info();
                    int signed_a = int8_t(a), signed_b = int8_t(b);
                    int signed_result = subtract ? signed_a - signed_b : signed_a + signed_b;
                    uint8_t value = subtract ? a - b : a + b;
                    unsigned expected = (value & 0x80 ? 0x80 : 0) | (signed_result < -128 || signed_result > 127 ? 0x40 : 0) | (value == 0 ? 2 : 0) | ((subtract ? a < b : a + b > 255) ? 1 : 0);
                    require(uint8_t(info.reg[0]) == value && (info.sr & 0xc3) == expected, "byte arithmetic flags incorrect");
                }
        f.program({0xc1, 0x3a, 0x7f, 0xff, 0xc1, 0x2f, 0, 1, 0, 0});
        info = f.run();
        require(uint16_t(info.reg[0]) == 0x8000 && (info.sr & 0xc3) == 0xc0, "word overflow");
        f.program({0xc1, 0x3a, 0xff, 0xff, 0xc1, 0x2f, 0, 1, 0, 0});
        info = f.run();
        require(info.reg[0] == 0 && (info.sr & 0xc3) == 3, "word carry");
        f.program({0xc1, 0x3a, 0x80, 0, 0xc1, 0x20, 0x7f, 0xff, 0x80, 0x28, 0xff, 0x10, 0xc9, 0x3a, 0, 0xff, 0, 0});
        info = f.run();
        require(info.reg[1] == 0, "signed branch after overflowed comparison");
        f.program({0x08, 0x06, 0xaa, 0x10, 0x06, 0x12, 0x34, 0x09, 0x07, 0x02, 0x07, 0, 0});
        info = f.run();
        require(uint16_t(info.reg[1]) == 0x1234 && uint8_t(info.reg[0]) == 0xaa && info.sp == 0x3000, "stack widths/order");
        f.program({0, 0x0e, 0x80, 0x04, 0xff, 0x0a, 0, 0x0f, 0, 0, 0xc1, 0x3a, 0x12, 0x34, 0x08, 0x06, 0xaa, 0, 0x05});
        info = f.run();
        require(uint16_t(info.reg[0]) == 0x1234 && info.sp == 0x3000 && info.bp == 0x3000, "call/ret locals and explicit BP preservation");
        f.program({0xc1, 0x3a, 0x12, 0x34, 0xc0, 0x3b, 0x40, 0, 0xc8, 0x3a, 0x40, 0, 0x0e, 0x3b, 0x40, 2, 0xab, 0xcd, 0, 0});
        info = f.run();
        require(uint16_t(info.reg[1]) == 0x1234 && f.cpu.peek(0x4002) == 0xab && f.cpu.peek(0x4003) == 0xcd, "big endian memory/store immediates");
        // Relative byte addresses use PC after the complete instruction.
        f.program({0xc6, 0x3a, 0x02, 0, 0, 0x55, 0xaa});
        info = f.run();
        require(uint8_t(info.reg[0]) == 0x55, "relative addressing origin");
        f.program({0x0e, 0x3b, 0x40, 0, 0x40, 2, 0x82, 0x22, 0x40, 0});
        f.cpu.load({0xc1, 0x3a, 0x66, 0x77, 0, 0}, 0x4002);
        info = f.run();
        require(uint16_t(info.reg[0]) == 0x6677, "indirect branch");
        // Protected absolute addressing is rebased, not raw host memory access.
        f.program({0x10, 0x50, 0x40, 0, 0x10, 0x51, 0x40, 0xff, 0x80, 0x22, 0x40, 0});
        f.cpu.load({0, 0x15, 0xc0, 0x3a, 0, 0x20}, 0x4000);
        f.cpu.poke(0x4020, 0x12);
        f.cpu.poke(0x4021, 0x34);
        f.run(6);
        info = f.cpu.get_info();
        require(uint16_t(info.reg[0]) == 0x1234 && !(info.sr & 8), "MTU translation");
        // A user cannot set privilege/interrupt bits through LDSR or POSR.
        f.program({0x80, 0x22, 0x40, 0});
        f.cpu.load({0, 0x15, 0x08, 0x3e, 0xff}, 0x4000);
        f.cpu.step();
        f.cpu.step();
        f.cpu.step();
        require(!(f.cpu.get_info().sr & 8) && f.cpu.error().empty(), "LDSR privilege escalation");
        f.program({0x80, 0x22, 0x40, 0});
        f.cpu.load({0, 0x15, 0, 0x12}, 0x4000);
        f.cpu.poke(4, 0x41);
        f.cpu.poke(5, 0);
        f.cpu.load({0, 0}, 0x4100);
        f.cpu.step();
        f.cpu.step();
        f.cpu.step();
        info = f.cpu.get_info();
        require(info.pc == 0x4100 && info.reg[0] == 1 && (info.sr & 8) && !(info.sr & 0x30), "privilege NMI entry");
        require(f.cpu.peek(0x3000) == 0x40 && f.cpu.peek(0x3001) == 2 && f.cpu.peek(0x3003) == 0, "NMI saved state");
        f.program({0, 3});
        f.cpu.poke(6, 0x40);
        f.cpu.poke(7, 0);
        f.cpu.step();
        info = f.cpu.get_info();
        require(info.pc == 0x4000 && info.sp == 0x3003 && info.bp == 0x3000, "syscall frame");
        f.program({0xff, 0xff});
        f.cpu.poke(4, 0);
        f.cpu.poke(5, 0);
        f.cpu.step();
        require(f.cpu.halted() && !f.cpu.error().empty(), "unhandled invalid opcode must halt diagnostically");
        auto echo = std::make_unique<Echo>(&f.mapper, f.flags);
        auto *device = echo.get();
        f.mapper.attach(0x100, std::move(echo));
        f.program({0xc9, 0x3a, 0x56, 0x78, 0x80, 0x1e, 1, 0, 0, 0x1f, 0x0e, 0x1b, 1, 0, 0xab, 0xcd, 0x80, 0x19, 1, 0, 0, 0});
        info = f.run();
        require(uint16_t(info.reg[0]) == 0x102 && uint16_t(info.reg[1]) == 0x100 && device->last == 0xabcd && device->additional, "device IO/information request");
        f.program({0x08, 0x60, 1, 0, 0x61});
        f.cpu.poke(8, 0x40);
        f.cpu.poke(9, 0);
        f.cpu.load({0, 0}, 0x4000);
        f.cpu.step();
        f.cpu.step();
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        f.cpu.step();
        require(f.cpu.halted() && f.cpu.error().empty(), "timer interrupt");
        AVC64 gpu(&f.mapper, f.flags);
        gpu.cpu_write(0x0305, false);
        gpu.cpu_write(0x0406, false);
        gpu.cpu_write(0x05ab, false);
        gpu.cpu_write(0x0000, false);
        require(gpu.cpu_read(0, false) == 0xab && gpu.cpu_read(0x0300, false) == 5, "AVC64 pixel and coordinate registers");
        gpu.cpu_write(0x0501, false);
        gpu.cpu_write(0x0200, false);
        require(gpu.cpu_read(0, false) == 1, "AVC64 clear");
        gpu.load_textures({0, 1, 2, 1, 0, 0, 0x80}); // two pixels: foreground, background
        gpu.cpu_write(0x0707, false);
        gpu.cpu_write(0x0808, false);
        gpu.cpu_write(0x0500, false);
        gpu.cpu_write(0x0601, false);
        gpu.cpu_write(0x0100, false);
        require(gpu.cpu_read(0, false) == 7, "AVC64 texture foreground");
        gpu.cpu_write(0x0306, false);
        require(gpu.cpu_read(0, false) == 8, "AVC64 texture background");
        bool truncated = false;
        try
        {
            gpu.load_textures({0, 2, 2, 1, 0, 4, 0xff});
        }
        catch (const std::exception &)
        {
            truncated = true;
        }
        require(truncated, "Truncated texture must be rejected");
        f.program({0, 0});
        f.cpu.start();
        f.cpu.wait();
        require(f.cpu.halted(), "worker lifecycle");
        f.cpu.load_init_state();
        f.cpu.start();
        f.cpu.wait();
        require(f.cpu.halted(), "worker restart after reset");
        std::cout << "CPU tests passed (131072 byte arithmetic cases plus execution, memory, stack, IO, interrupt and timer regressions)\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
