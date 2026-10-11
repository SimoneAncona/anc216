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
            throw std::runtime_error("Expected OS image directory");
        const std::string directory = argv[1];
        std::ifstream boot_file(directory + "/boot.bin", std::ios::binary);
        const std::vector<unsigned char> boot{std::istreambuf_iterator<char>(boot_file), {}};
        for (const std::string name : {"large", "limit", "overflow", "symbols"})
        {
            ANC216::EmuFlags flags;
            flags.novideo = flags.debug_mode = true;
            flags.bootfile = directory + "/boot.bin";
            flags.inserts.emplace_back(0x0100, directory + "/system.rom");
            flags.cards.emplace_back(0x4000, directory + "/loader-" + name + ".afs");
            ANC216::EmemMapper devices(flags);
            devices.attach(DEFAULT_VIDEO_CARD_ADDR, std::make_unique<ANC216::AVC64>(&devices, flags));
            ANC216::CPU cpu(&devices, flags);
            for (unsigned cycle = 0; cycle < 4000000 && !cpu.halted(); ++cycle)
            {
                cpu.step();
                if (cpu.peek(0x00f1) == 0x61)
                    break;
            }
            if (cpu.halted() || !cpu.error().empty())
                throw std::runtime_error(name + ": " + cpu.error());
            if (name == "overflow")
            {
                if (cpu.peek(0x00f1) != 0x60 || uint16_t(cpu.get_info().reg[7]) != 7)
                    throw std::runtime_error("Oversized payload was not rejected before launch");
                if (cpu.peek(0x4000) != 0)
                    throw std::runtime_error("Rejected payload modified user memory");
            }
            else if (cpu.peek(0x00f1) != 0x61 || cpu.peek(0x00f3) != 42)
                throw std::runtime_error(name + ": payload did not load and execute correctly");
            for (unsigned i = 0; i < boot.size(); ++i)
                if (cpu.peek(0xff00 + i) != boot[i])
                    throw std::runtime_error(name + ": firmware was overwritten");
        }
        std::cout << "Streaming loader: large payload, exact limit, overflow and large symbols passed\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
