#include <emem.hh>
#include <cpu.hh>
#include <avc64.hh>
#include <keyboard.hh>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace
{
    class MpmeDevice : public ANC216::Device
    {
        std::array<uint8_t, MAX_MEM> memory{};
        uint16_t pointer = 0;
        std::fstream backing; // Writable chips persist bytes before WRITE returns.

    public:
        MpmeDevice(ANC216::EmemMapper *mapper, const ANC216::EmuFlags &flags,
                     const std::string &filename) : Device(mapper, flags)
        {
            id = ANC216::MPME_CARD;
            std::ifstream in(filename, std::ios::binary);
            if (!in)
                throw std::runtime_error("Cannot open device image: " + filename);
            std::vector<uint8_t> bytes(std::istreambuf_iterator<char>(in), {});
            if (bytes.size() > memory.size())
                throw std::runtime_error("Device image exceeds 64 KiB");
            std::copy(bytes.begin(), bytes.end(), memory.begin());
            backing.open(filename, std::ios::in | std::ios::out | std::ios::binary);
            if (!backing)
                throw std::runtime_error("Cannot open writable card: " + filename);
        }
        void cpu_write(uint16_t value, bool additional) override
        {
            if (!additional)
            {
                pointer = value;
                return;
            }
            const unsigned count = transfer_width == 1 ? 1 : pointer == 0xffff ? 0 : 2;
            if (!count)
                return; // A word cannot cross the chip's final byte.
            const char bytes[] = {char(count == 1 ? value : value >> 8), char(value)};
            backing.seekp(pointer);
            backing.write(bytes, count);
            backing.flush();
            if (!backing)
                throw std::runtime_error("Failed to persist MPME write");
            // Publish the in-memory state only after the host write succeeds.
            for (unsigned i = 0; i < count; ++i)
                memory[pointer + i] = static_cast<unsigned char>(bytes[i]);
        }
        uint16_t cpu_read(uint16_t value, bool) override
        {
            pointer = value;
            if (transfer_width == 1)
                return memory[pointer];
            return (unsigned(memory[pointer]) << 8) | (pointer == 0xffff ? 0 : memory[pointer + 1]);
        }
    };
} // namespace
using namespace ANC216;
EmemMapper::EmemMapper(const EmuFlags &flags, Video::Window *window)
{
#ifdef ANC216_WITH_SDL
    if (!flags.novideo && window)
        attach(DEFAULT_VIDEO_CARD_ADDR, std::make_unique<AVC64>(this, flags, window));
#else
    (void)window;
    if (!flags.novideo)
        throw std::runtime_error("Video requires a build configured with -DANC216_WITH_SDL=ON; use --novideo");
#endif
    if (!flags.nokeyboard)
        attach(DEFAULT_KEYBOARD_ADDR, std::make_unique<Keyboard>(this, flags));
    for (const auto &[address, file] : flags.inserts)
        map_rom(address, file);
    for (const auto &[address, file] : flags.cards)
        attach(address, std::make_unique<MpmeDevice>(this, flags, file));
}
void EmemMapper::map_rom(uint16_t base, const std::string &filename)
{
    std::ifstream in(filename, std::ios::binary);
    if (!in)
        throw std::runtime_error("Cannot open ROM image: " + filename);
    const std::vector<uint8_t> bytes(std::istreambuf_iterator<char>(in), {});
    if (bytes.empty() || bytes.size() > MAX_MEM - unsigned(base))
        throw std::runtime_error("ROM image is empty or extends past EMEM 0xffff");
    // Validate the entire range before publishing any mapped cells.
    for (unsigned offset = 0; offset < bytes.size(); ++offset)
        if (devices[base + offset] || rom_mapped[base + offset])
            throw std::runtime_error("Overlapping EMEM ROM/device mappings");
    for (unsigned offset = 0; offset < bytes.size(); ++offset)
    {
        rom_bytes[base + offset] = bytes[offset];
        rom_mapped[base + offset] = true;
    }
}
bool EmemMapper::is_rom(uint16_t address) const
{
    return rom_mapped[address];
}
EmemMapper::~EmemMapper() = default;
void EmemMapper::set_cpu(CPU *value)
{
    cpu = value;
}
void EmemMapper::attach(uint16_t address, std::unique_ptr<Device> device)
{
    if (devices[address] || rom_mapped[address])
        throw std::runtime_error("Overlapping EMEM ROM/device mappings");
    if (auto *display = dynamic_cast<AVC64 *>(device.get()))
        displays.push_back(display);
    devices[address] = std::move(device);
}
uint16_t EmemMapper::where_am_i(const Device *device) const
{
    for (unsigned i = 0; i < devices.size(); ++i)
        if (devices[i].get() == device)
            return i;
    throw std::runtime_error("Device is not attached");
}
void EmemMapper::write(uint16_t value, uint16_t address, bool additional, unsigned width, bool)
{
    if (auto *d = devices[address].get())
    {
        d->set_transfer_width(width);
        d->cpu_write(value, additional);
    }
}
uint16_t EmemMapper::read(uint16_t value, uint16_t address, bool additional, unsigned width)
{
    if (rom_mapped[address])
    {
        // ROM ignores R1/request payload: the bus address selects the byte.
        if (width == 1)
            return rom_bytes[address];
        const unsigned next = unsigned(address) + 1;
        return (unsigned(rom_bytes[address]) << 8) |
               (next < MAX_MEM && rom_mapped[next] ? rom_bytes[next] : 0);
    }
    if (auto *d = devices[address].get())
    {
        d->set_transfer_width(width);
        return d->cpu_read(value, additional);
    }
    return 0;
}
uint16_t EmemMapper::info_req(uint16_t address)
{
    return rom_mapped[address] ? ROM : devices[address] ? devices[address]->cpu_info_req() : 0xffff;
}
void EmemMapper::request(uint16_t address, uint16_t value, bool additional, bool)
{
    auto data = read(value, address, additional);
    if (cpu)
        cpu->einr(address, data, additional ? 2 : 1);
}

void EmemMapper::present()
{
    for (auto *display : displays)
        display->present();
}

void EmemMapper::keyboard_input(uint16_t value)
{
    if (auto *keyboard = dynamic_cast<Keyboard *>(devices[DEFAULT_KEYBOARD_ADDR].get()))
        keyboard->input(value);
}
void EmemMapper::pump_keyboard()
{
    // Called under CPU's recursive lock, before executing an instruction.
    // Queue writers only take the keyboard lock; never hold it while entering CPU.
    if (auto *keyboard = dynamic_cast<Keyboard *>(devices[DEFAULT_KEYBOARD_ADDR].get()))
        if (auto value = keyboard->pending(); value && cpu && (cpu->peek(2) || cpu->peek(3)) && cpu->einr(DEFAULT_KEYBOARD_ADDR, value, 1))
            keyboard->consume();
}
