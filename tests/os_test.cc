#include <cpu.hh>
#include <emem.hh>
#include <avc64.hh>
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv)
{
    try
    {
        if (argc != 2)
            throw std::runtime_error("Expected OS build directory");
        const std::string directory = argv[1];
        ANC216::EmuFlags flags;
        flags.novideo = true;
        flags.debug_mode = true;
        flags.bootfile = directory + "/boot.bin";
        flags.charmap = directory + "/charmap.bin";
        flags.inserts.emplace_back(0x0100, directory + "/system.rom");
        ANC216::EmemMapper mapper(flags);
        auto gpu = std::make_unique<ANC216::AVC64>(&mapper, flags);
        auto *display = gpu.get();
        mapper.attach(DEFAULT_VIDEO_CARD_ADDR, std::move(gpu));
        ANC216::CPU cpu(&mapper, flags);
        for (unsigned cycle = 0; cycle < 20000 && cpu.peek(0x00f0) == 0 && !cpu.halted(); ++cycle)
            cpu.step();
        if (cpu.halted() || cpu.peek(0x00f0) != 0x21 || cpu.peek(0x00f1) != 0x60)
            throw std::runtime_error("Kernel failed to become ready: " + cpu.error());
        if (cpu.get_info().sp != 0x3000 || cpu.get_info().bp != 0x3000)
            throw std::runtime_error("Console did not restore the stack");
        // The first glyph is A: 01110 in row zero, centered in an 8-pixel cell.
        for (unsigned x = 0; x < 8; ++x)
        {
            display->cpu_write(0x0300 | x, false);
            display->cpu_write(0x0400, false);
            if (display->cpu_read(0, false) != (x >= 2 && x <= 4 ? 0xff : 0))
                throw std::runtime_error("Incorrect banner glyph pixels");
        }
        for (unsigned cycle = 0; cycle < 100; ++cycle)
            cpu.step();
        if (cpu.halted() || !cpu.error().empty())
            throw std::runtime_error("Kernel idle loop failed");
        std::cout << "OS ROM boot, console pixels, stack and idle passed\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
