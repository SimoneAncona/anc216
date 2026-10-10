#include <cpu.hh>
#include <emem.hh>
#include <fstream>
#include <iostream>
#include <iterator>

int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    std::ifstream input(argv[1], std::ios::binary);
    if (!input) return 2;
    std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(input), {}};
    ANC216::EmuFlags flags;
    flags.novideo = flags.noaudio = flags.nokeyboard = true;
    flags.debug_mode = true;
    ANC216::EmemMapper memory(flags);
    ANC216::CPU cpu(&memory, flags);
    cpu.load(bytes, 0);
    cpu.debug_set("pc", 0x100);
    cpu.debug_set("r4", 0x1234);
    cpu.debug_set("r5", 0xabcd);
    cpu.debug_set("r6", 0x6789);
    for (unsigned step = 0; step < 100000 && !cpu.halted(); ++step) cpu.step();
    const auto result = cpu.get_info();
    const auto expected = uint16_t(std::stoul(argv[2], nullptr, 0));
    if (!cpu.halted() || !cpu.error().empty() || uint16_t(result.reg[0]) != expected ||
        result.sp != 0x3000 || result.bp != 0x3000 || uint16_t(result.reg[4]) != 0x1234 ||
        uint16_t(result.reg[5]) != 0xabcd || uint16_t(result.reg[6]) != 0x6789)
    {
        std::cerr << "Guest failed: r0=" << uint16_t(result.reg[0]) << " expected=" << expected
                  << " sp=" << result.sp << " bp=" << result.bp << " error=" << cpu.error() << '\n';
        return 1;
    }
    return 0;
}
