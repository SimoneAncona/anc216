#include <cpu.hh>
#include <emem.hh>
#include <avc64.hh>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <chrono>
#include <thread>
#include <fstream>
#include <vector>

// Drive real SDL-style keyboard events through shell/getl, then execute the
// separate command UALf on the guest. No host filesystem command implements it.
int main(int argc, char **argv)
{
    try
    {
        if (argc != 2)
            throw std::runtime_error("Expected OS image directory");
        const std::string directory = argv[1];
        ANC216::EmuFlags flags;
        flags.novideo = flags.debug_mode = true;
        flags.bootfile = directory + "/boot.bin";
        flags.charmap = directory + "/charmap.bin";
        flags.inserts.emplace_back(0x0100, directory + "/system.rom");
        flags.cards.emplace_back(0x4000, directory + "/shell-test.afs");
        flags.cards.emplace_back(0x3001, directory + "/disk1.afs");
        // Persistent guest writes use private copies, never shared build fixtures.
        for (auto &[address, image] : flags.cards)
        {
            const auto copy = directory + "/shell-volume-" + std::to_string(address) + ".afs";
            std::filesystem::copy_file(image, copy, std::filesystem::copy_options::overwrite_existing);
            image = copy;
        }
        ANC216::EmemMapper devices(flags);
        auto gpu = std::make_unique<ANC216::AVC64>(&devices, flags);
        auto *display = gpu.get();
        devices.attach(DEFAULT_VIDEO_CARD_ADDR, std::move(gpu));
        ANC216::CPU cpu(&devices, flags);
        auto ready = [&]
        {
            for (unsigned i = 0; i < 1500000 && !cpu.halted(); ++i)
            {
                if (cpu.peek(0x6200) == 0x53 && cpu.peek(0x6201) == 0x48)
                    return;
                cpu.step();
            }
            throw std::runtime_error("Shell did not return to input, PC=" + std::to_string(cpu.get_pc()) + " " + cpu.error());
        };
        ready();
        auto pixel = [&](unsigned x, unsigned y)
        {
            display->cpu_write(0x0300 | x, false);
            display->cpu_write(0x0400 | y, false);
            return display->cpu_read(0, false);
        };
        // Capture the initial prompt pixels, excluding the blinking cursor row.
        std::vector<uint16_t> prompt_pixels;
        for (unsigned y = 0; y < 7; ++y)
            for (unsigned x = 0; x < 24; ++x)
                prompt_pixels.push_back(pixel(x, y));
        auto steps = [&](unsigned count)
        {
            for (unsigned i = 0; i < count && !cpu.halted(); ++i)
                cpu.step();
        };
        steps(2000); // complete getl setup after the shell's ready marker
        if (pixel(24, 7) != 0xff)
            throw std::runtime_error("Input cursor did not appear at the prompt");
        std::this_thread::sleep_for(std::chrono::milliseconds(550));
        steps(2000);
        if (pixel(24, 7) != 0)
            throw std::runtime_error("Input cursor did not blink off");
        devices.keyboard_input('A');
        steps(2000);
        if (pixel(26, 0) != 0xff)
            throw std::runtime_error("Input glyph was not drawn");
        devices.keyboard_input(8);
        steps(2000);
        if (pixel(26, 0) != 0 || pixel(24, 7) != 0xff)
            throw std::runtime_error("Backspace did not erase the glyph and move the cursor");
        // Wrap from the last cell of row zero, then delete back across the wrap.
        for (unsigned i = 0; i < 29; ++i)
            devices.keyboard_input('A');
        steps(50000);
        devices.keyboard_input(8);
        steps(2000);
        if (pixel(250, 0) != 0 || pixel(248, 7) != 0xff)
            throw std::runtime_error("Backspace did not restore the previous row's cell");
        for (unsigned i = 0; i < 28; ++i)
            devices.keyboard_input(8);
        steps(50000);
        auto command = [&](const std::string &line, unsigned expected = 0)
        {
            std::cout << "shell: " << line << std::endl;
            for (char ch : line + "\n")
                devices.keyboard_input(ch);
            for (unsigned i = 0; i < 1500000 && cpu.peek(0x6200) == 0x53 && !cpu.halted(); ++i)
                cpu.step();
            if (cpu.peek(0x6200) == 0x53)
                throw std::runtime_error("Shell did not consume its input: " + line);
            ready();
            if (cpu.peek(0x00f2) != 0 || cpu.peek(0x00f3) != expected)
                throw std::runtime_error("Unexpected command exit: " + line + " status=" + std::to_string(cpu.peek(0x00f3)));
        };
        auto disk_byte = [&](unsigned at)
        {
            return unsigned(devices.read(at, 0x4000) >> 8);
        };
        auto directory_id = [&](const std::string &name)
        {
            for (unsigned id = 1; id <= 96; ++id)
            {
                const unsigned at = 649 + (id - 1) * 16;
                std::string stored;
                for (unsigned i = 1; i <= 15 && disk_byte(at + i); ++i)
                    stored += char(disk_byte(at + i));
                if (disk_byte(at) == 0 && stored == name)
                    return id;
            }
            return 0u;
        };
        auto file_id = [&](unsigned parent, const std::string &name)
        {
            for (unsigned id = 1; id <= 196; ++id)
            {
                const unsigned metadata = devices.read(257 + (id - 1) * 2, 0x4000);
                if (metadata != (parent << 8 | 1))
                    continue;
                const unsigned at = 2185 + (id - 1) * 323;
                std::string stored;
                for (unsigned i = 0; i < 17 && disk_byte(at + i); ++i)
                    stored += char(disk_byte(at + i));
                if (disk_byte(at + 17))
                    stored += '.';
                for (unsigned i = 17; i < 20 && disk_byte(at + i); ++i)
                    stored += char(disk_byte(at + i));
                if (stored == name)
                    return id;
            }
            return 0u;
        };
        auto cwd = [&]
        {
            std::string result;
            for (unsigned at = 0x5100; cpu.peek(at) && result.size() < 255; ++at)
                result += char(cpu.peek(at));
            return result;
        };

        // Compare actual console pixels before and after every single-row scroll.
        std::ifstream font_file(directory + "/charmap.bin", std::ios::binary);
        const std::vector<unsigned char> font((std::istreambuf_iterator<char>(font_file)), {});
        auto glyph = [&](char ch, unsigned column, unsigned row)
        {
            const unsigned at = (unsigned(ch) - 32) * 13 + 5;
            for (unsigned y = 0; y < 8; ++y)
                for (unsigned x = 0; x < 8; ++x)
                    if (pixel(column * 8 + x, row * 8 + y) !=
                        ((font.at(at + y) & (0x80 >> x)) ? 0xff : 0))
                        throw std::runtime_error("more rendered an incorrect glyph: " + std::string(1, ch) + " column=" + std::to_string(column) + " row=" + std::to_string(row));
        };
        // Prompts stay visible and never enter the redirected file, even across reloads.
        command("touch /prompt-test.txt");
        const unsigned prompt_file = file_id(0, "prompt-test.txt");
        if (!prompt_file)
            throw std::runtime_error("Missing prompt redirection target");
        const unsigned prompt_base = 2185 + (prompt_file - 1) * 323;
        command("redct /prompt-test.txt");
        command("clear");
        for (unsigned y = 0; y < 7; ++y)
            for (unsigned x = 0; x < 24; ++x)
                if (pixel(x, y) != prompt_pixels.at(y * 24 + x))
                    throw std::runtime_error("Redirected shell prompt disappeared from the console");
        if (devices.read(prompt_base + 21, 0x4000) != 0)
            throw std::runtime_error("Shell prompt entered the redirected file");
        command("");
        if (devices.read(prompt_base + 21, 0x4000) != 0)
            throw std::runtime_error("Repeated prompt entered the redirected file");
        command("pwd");
        if (devices.read(prompt_base + 21, 0x4000) != 2 ||
            disk_byte(prompt_base + 23) != '/' || disk_byte(prompt_base + 24) != '\n')
            throw std::runtime_error("Prompt bypass disturbed command redirection");
        command("redct");
        command("rm /prompt-test.txt");
        // Echo intentionally has console permission only; redct authorized the file.
        command("touch test.txt");
        const unsigned echo_file = file_id(0, "test.txt");
        if (!echo_file)
            throw std::runtime_error("Missing echo redirection target");
        const unsigned echo_base = 2185 + (echo_file - 1) * 323;
        command("redct test.txt");
        command("echo ciao");
        command("echo hello");
        const std::string expected_echo = "ciao\nhello\n";
        if (devices.read(echo_base + 21, 0x4000) != expected_echo.size())
            throw std::runtime_error("Redirected echo wrote an incorrect file length");
        for (unsigned i = 0; i < expected_echo.size(); ++i)
            if (disk_byte(echo_base + 23 + i) != unsigned(expected_echo[i]))
                throw std::runtime_error("Redirected echo lost its arguments or newline");
        command("redct");
        command("cat test.txt");
        if (devices.read(echo_base + 21, 0x4000) != expected_echo.size())
            throw std::runtime_error("Stopping redirection changed the file");
        command("rm test.txt");
        command("more", 1);
        command("more /missing", 3);
        command("more /more-exact.txt"); // EOF needs no extra Enter.
        for (bool wrapped : {false, true})
        {
            const std::string line = wrapped ? "more /more-wrap.txt" : "more /more-lines.txt";
            for (char ch : line + "\n")
                devices.keyboard_input(ch);
            steps(1500000);
            for (unsigned scroll = 0; scroll <= 27; ++scroll)
            {
                if (cpu.halted() || cpu.peek(0x6200) == 0x53)
                    throw std::runtime_error("more did not wait after one row");
                for (unsigned row = 0; row < 27; ++row)
                    for (unsigned col = 0; col < (wrapped ? 32u : 1u); ++col)
                        glyph(row < 27 - scroll ? 'A' : 'B', col, row);
                glyph('P', 0, 27);
                devices.keyboard_input('x');
                steps(2000);
                glyph(scroll < 27 ? 'A' : 'B', 0, 0);
                devices.keyboard_input(10);
                steps(500000);
            }
            ready();
            for (unsigned row = 0; row < 26; ++row)
                glyph('B', 0, row);
            glyph('C', 0, 26); // EOF leaves the scrolled text visible.
            if (cpu.peek(0x00f3) != 0)
                throw std::runtime_error("more failed after paging");
        }
        command("edit"); // C syscall wrapper and return through logical frames.
        for (char ch : std::string("help\n")) devices.keyboard_input(ch);
        steps(1500000);
        for (unsigned page = 0; page < 128 && cpu.peek(0x6200) != 0x53; ++page) {
            devices.keyboard_input(10);
            steps(500000);
        }
        ready();
        command("test-strings");
        command("test-fs_namespace");
        command("test-video");
        command("lscpu");
        command("fstat /data/message.txt");
        command("fstat /data");
        command("mkdir /work");
        const auto work = directory_id("work");
        if (!work)
            throw std::runtime_error("mkdir did not allocate its directory metadata");
        command("cd /work");
        if (cwd() != "/work")
            throw std::runtime_error("Cwd did not survive reloading init");
        command("touch note.txt");
        const auto note = file_id(work, "note.txt");
        if (!note || devices.read(2185 + (note - 1) * 323 + 21, 0x4000) != 0)
            throw std::runtime_error("touch did not allocate a zero-length file head");
        command("ls");
        command("cat note.txt");
        command("touch note.txt");
        command("mkdir note.txt", 10);
        command("rm /work", 11);
        command("pwd");
        command("echo hello world");
        command("cd ../data/./");
        command("cat message.txt");
        command("touch message.txt");
        command("cat message.txt");
        if (cwd() != "/data")
            throw std::runtime_error("Dot/parent path normalization failed");
        command("cd /work");
        command("rm note.txt");
        if (file_id(work, "note.txt"))
            throw std::runtime_error("rm did not free the file allocation");
        command("cd /");
        command("rm work");
        if (directory_id("work"))
            throw std::runtime_error("rm did not release an empty directory");
        command("ls /");
        command("clear");
        command("mount 12289");
        command("cat /other.txt");
        command("mount 16384");
        command("ls /bin");
        command("touch /Case");
        command("touch /case");
        if (!file_id(0, "Case") || !file_id(0, "case") || file_id(0, "Case") == file_id(0, "case"))
            throw std::runtime_error("Case-sensitive names did not remain distinct");
        command("rm /Case");
        command("rm /case");
        command("touch /missing/file", 3);
        command("mkdir /bin", 10);
        command("rm /", 1);
        command("touch /bad:", 1);
        command("touch /bad.", 1);
        command("touch /12345678901234567.xyz");
        command("rm /12345678901234567.xyz");
        if (cpu.halted() || !cpu.error().empty())
            throw std::runtime_error(cpu.error());
        std::cout << "Separate shell commands, cwd, listing, creation and removal passed\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
