#include <emem.hh>
#include <cpu.hh>
#include <avc64.hh>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace
{
    class MemoryDevice : public ANC216::Device
    {
        std::array<uint8_t, MAX_MEM> memory{};
        uint16_t pointer = 0;
        bool readonly;

    public:
        MemoryDevice(ANC216::EmemMapper *mapper, const ANC216::EmuFlags &flags,
                     const std::string &filename, bool rom) : Device(mapper, flags), readonly(rom)
        {
            id = rom ? ANC216::ROM : ANC216::MPME_CARD;
            std::ifstream in(filename, std::ios::binary);
            if (!in)
                throw std::runtime_error("Cannot open device image: " + filename);
            std::vector<uint8_t> bytes(std::istreambuf_iterator<char>(in), {});
            if (bytes.size() > memory.size())
                throw std::runtime_error("Device image exceeds 64 KiB");
            std::copy(bytes.begin(), bytes.end(), memory.begin());
        }
        void cpu_write(uint16_t value, bool additional) override
        {
            if (!additional)
            {
                pointer = value;
                return;
            }
            if (readonly)
                return;
            if (transfer_width == 1)
                memory[pointer] = value;
            else if (pointer != 0xffff)
            {
                memory[pointer] = value >> 8;
                memory[pointer + 1] = value;
            }
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
    for (const auto &[address, file] : flags.inserts)
        attach(address, std::make_unique<MemoryDevice>(this, flags, file, true));
    for (const auto &[address, file] : flags.cards)
        attach(address, std::make_unique<MemoryDevice>(this, flags, file, false));
}
EmemMapper::~EmemMapper() = default;
void EmemMapper::set_cpu(CPU *value)
{
    cpu = value;
}
void EmemMapper::attach(uint16_t address, std::unique_ptr<Device> device)
{
    if (devices[address])
        throw std::runtime_error("Two devices mapped to the same EMEM address");
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
    if (auto *d = devices[address].get())
    {
        d->set_transfer_width(width);
        return d->cpu_read(value, additional);
    }
    return 0;
}
uint16_t EmemMapper::info_req(uint16_t address)
{
    return devices[address] ? devices[address]->cpu_info_req() : 0xffff;
}
void EmemMapper::request(uint16_t address, uint16_t value, bool additional, bool)
{
    auto data = read(value, address, additional);
    if (cpu)
        cpu->einr(address, data, additional ? 2 : 1);
}

void EmemMapper::present()
{
    for (auto &device : devices)
        if (auto *gpu = dynamic_cast<AVC64 *>(device.get()))
            gpu->present();
}
