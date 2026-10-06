#pragma once
#include <common.hh>
#include <device.hh>
#include <array>
#include <memory>
namespace ANC216
{
    namespace Video
    {
        class Window;
    }
    class EmemMapper
    {
        std::array<std::unique_ptr<Device>, MAX_MEM> devices{};
        CPU *cpu = nullptr;
        std::vector<AVC64 *> displays;

    public:
        EmemMapper(const EmuFlags &, Video::Window * = nullptr);
        ~EmemMapper();
        void set_cpu(CPU *);
        void present();
        void keyboard_input(uint16_t);
        void pump_keyboard();
        void attach(uint16_t, std::unique_ptr<Device>);
        uint16_t where_am_i(const Device *) const;
        void write(uint16_t value, uint16_t address, bool additional = false, unsigned width = 2, bool high_priority = false);
        uint16_t read(uint16_t value, uint16_t address, bool additional = false, unsigned width = 2);
        uint16_t info_req(uint16_t address);
        void request(uint16_t address, uint16_t value, bool additional = false, bool high_priority = false);
    };
} // namespace ANC216
