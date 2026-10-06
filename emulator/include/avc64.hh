#pragma once
#include <device.hh>
#include <video.hh>
#include <array>
#include <map>
#include <mutex>
#include <vector>
namespace ANC216
{
    class AVC64 : public Device
    {
        struct Texture
        {
            unsigned width, height, mode;
            std::vector<uint8_t> data;
        };
        Video::Window *window;
        std::array<uint8_t, 6> registers{}; // X,Y,D1,D2,D3,D4
        std::vector<uint8_t> pixels = std::vector<uint8_t>(256 * 224, 0);
        std::map<uint16_t, Texture> textures;
        std::mutex mutex;
        bool dirty = true;
        void draw_texture();

    public:
        AVC64(EmemMapper *, EmuFlags, Video::Window * = nullptr);
        void cpu_write(uint16_t, bool) override;
        uint16_t cpu_read(uint16_t, bool) override;
        void load_textures(const std::vector<uint8_t> &);
        void present();
    };
} // namespace ANC216
