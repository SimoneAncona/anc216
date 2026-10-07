#include <cpu.hh>
#include <emem.hh>
#include <avc64.hh>
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <algorithm>

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
        for (unsigned cycle = 0; cycle < 100000 && cpu.peek(0x00f0) == 0 && !cpu.halted(); ++cycle)
            cpu.step();
        if (cpu.halted() || cpu.peek(0x00f0) != 0x21 || cpu.peek(0x00f1) != 0x60)
            throw std::runtime_error("Kernel failed to become ready: " + cpu.error());
        if (cpu.get_info().sp != 0x3000 || cpu.get_info().bp != 0x3000)
            throw std::runtime_error("Console did not restore the stack");
        for (unsigned cycle = 0; cycle < 100; ++cycle)
            cpu.step();
        if (cpu.halted() || !cpu.error().empty())
            throw std::runtime_error("Kernel idle loop failed");
        flags.cards.emplace_back(0x3000, directory + "/demo-disk0.afs");
        flags.cards.emplace_back(0x3001, directory + "/disk1.afs");
        // Persistent guest writes use private copies, never shared build fixtures.
        for (auto &[address, image] : flags.cards)
        {
            const auto copy = directory + "/os-volume-" + std::to_string(address) + ".afs";
            std::filesystem::copy_file(image, copy, std::filesystem::copy_options::overwrite_existing);
            image = copy;
        }
        ANC216::EmemMapper disks(flags);
        auto process_gpu = std::make_unique<ANC216::AVC64>(&disks, flags);
        auto *process_display = process_gpu.get();
        disks.attach(DEFAULT_VIDEO_CARD_ADDR, std::move(process_gpu));
        ANC216::CPU process(&disks, flags);
        for (char key : std::string("TEST\n"))
            disks.keyboard_input(key);
        bool entered_user = false;
        unsigned cycles = 0;
        for (; cycles < 500000 && !process.halted(); ++cycles)
        {
            process.step();
            entered_user |= !(process.get_info().sr & 8);
            if (process.peek(0x00f0) == 0x21 && (process.peek(0x00f1) == 0x61 || process.peek(0x00f1) == 0x62))
                break;
        }
        const auto state = process.get_info();
        if (!entered_user || process.peek(0x00f1) != 0x61 || process.peek(0x00f2) != 0 || process.peek(0x00f3) != 0)
        {
            std::cerr << "cycles=" << cycles << " pc=" << std::hex << state.pc << " marker=" << unsigned(process.peek(0x00f1))
                      << " exit=" << unsigned(process.peek(0x00f3)) << " sr=" << unsigned(state.sr) << " progress=" << unsigned(process.peek(0x6001)) << " failstatus=" << unsigned(process.peek(0x6003)) << " failcount=" << ((unsigned(process.peek(0x6004)) << 8) | process.peek(0x6005)) << " r6=" << uint16_t(state.reg[6]) << " r0=" << uint16_t(state.reg[0]) << " r1=" << uint16_t(state.reg[1]) << " r7=" << uint16_t(state.reg[7]) << '\n';
            throw std::runtime_error("User UALf/MPME/syscall integration failed: " + process.error());
        }
        for (unsigned cycle = 0; cycle < 20; ++cycle)
            process.step();
        const auto finished = process.get_info();
        if (process.peek(0x5000) != 'T' || process.peek(0x5003) != 'T' || process.peek(0x5004) != 0)
            throw std::runtime_error("Keyboard getl/exec did not preserve the typed line");
        // The first data byte was overwritten through fwrite, but host card files
        // are persisted to the private backing card before WRITE returns.
        if (!(finished.sr & 8) || finished.sp != 0x3000 || finished.bp != 0x3000)
            throw std::runtime_error("Exit did not return to the kernel stack");
        auto screen = [&]
        {
            std::vector<uint16_t> image;
            for (unsigned y = 0; y < 224; ++y)
            {
                process_display->cpu_write(0x0400 | y, false);
                for (unsigned x = 0; x < 256; ++x)
                {
                    process_display->cpu_write(0x0300 | x, false);
                    image.push_back(process_display->cpu_read(0, false));
                }
            }
            return image;
        };
        const auto idle_screen = screen();
        for (char key : std::string("XYZ"))
            disks.keyboard_input(key);
        for (unsigned cycle = 0; cycle < 1000; ++cycle)
            process.step();
        if (process.halted() || process.peek(0x00f7) != 'Z' || process.get_info().sp != 0x3000)
            throw std::runtime_error("Idle keyboard IRQ return/queued delivery failed");
        if (screen() != idle_screen)
            throw std::runtime_error("Idle keyboard input unexpectedly changed the console");
        process.request_soft_reset();
        for (unsigned cycle = 0; cycle < 250000; ++cycle)
            process.step();
        if (process.halted() || !process.error().empty() || process.peek(0x00f1) != 0x63)
            throw std::runtime_error("OS soft reset handler failed");
        process.request_shutdown();
        for (unsigned cycle = 0; cycle < 20 && !process.halted(); ++cycle)
            process.step();
        if (!process.halted() || process.peek(0x00f1) != 0x64 || !process.error().empty())
            throw std::runtime_error("OS shutdown handler failed");
        // Corrupt inputs are attached as raw MPME images, bypassing the host
        // cardreader deliberately so guest validation is exercised.
        std::ifstream source(directory + "/demo-disk0.afs", std::ios::binary);
        std::vector<uint8_t> original(std::istreambuf_iterator<char>(source), {});
        unsigned head = 0;
        for (unsigned id = 1; id <= 196; ++id)
        {
            unsigned offset = 2185 + (id - 1) * 323;
            if (original[257 + (id - 1) * 2 + 1] == 1 &&
                std::equal(original.begin() + offset, original.begin() + offset + 4, "init") &&
                original[offset + 4] == 0)
                head = id;
        }
        if (!head)
            throw std::runtime_error("Missing demo init cluster");
        const unsigned first = 2185 + (head - 1) * 323;
        for (unsigned scenario = 0; scenario < 3; ++scenario)
        {
            auto image = original;
            if (scenario == 0)
                image[first + 23] = 'X'; // Wrong UALf magic.
            else if (scenario == 1)
                image[first + 20] = head; // File chain points back to itself.
            else
            {
                const unsigned entry = (unsigned(image[first + 27]) << 8) | image[first + 28];
                // Map file offsets across AFS cluster payload boundaries.
                unsigned cluster = head, position = entry, payload = 300;
                while (position >= payload)
                {
                    position -= payload;
                    const unsigned base = 2185 + (cluster - 1) * 323;
                    cluster = image[base + (cluster == head ? 20 : 0)];
                    payload = 320;
                }
                const unsigned target = 2185 + (cluster - 1) * 323 + (cluster == head ? 23 : 3) + position;
                image[target] = 0;
                image[target + 1] = 0x12; // SETS is privileged: user must fault.
            }
            const std::string card = directory + "/negative-" + std::to_string(scenario) + ".afs";
            {
                std::ofstream output(card, std::ios::binary);
                output.write(reinterpret_cast<const char *>(image.data()), image.size());
            }
            auto test_flags = flags;
            test_flags.cards = {{0x3000, card}};
            ANC216::EmemMapper test_mapper(test_flags);
            ANC216::CPU test_cpu(&test_mapper, test_flags);
            for (unsigned cycle = 0; cycle < 1000000 && !test_cpu.halted(); ++cycle)
                test_cpu.step();
            if (test_cpu.halted() || !test_cpu.error().empty())
                throw std::runtime_error("Malformed guest input crashed the kernel");
            if (scenario < 2 && (test_cpu.peek(0x00f1) != 0x60 || uint16_t(test_cpu.get_info().reg[7]) != (scenario == 0 ? 7 : 5)))
                throw std::runtime_error("Bad UALf/AFS chain was not rejected");
            if (scenario == 2 && (test_cpu.peek(0x00f1) != 0x62 || test_cpu.peek(0x00f5) != 1))
                throw std::runtime_error("Privileged user opcode did not terminate through NMI");
        }
        std::cout << "OS boot, console, user UALf, two MPME volumes, FS read/write and syscalls passed\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
