#include <avc64.hh>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>
using namespace ANC216;
AVC64::AVC64(EmemMapper *mapper, EmuFlags flags, Video::Window *win) : Device(mapper, flags), window(win)
{
    id = AVC64_VIDEO_CARD;
#ifdef ANC216_WITH_SDL
    if (window)
    {
        window->init();
        window->change_logical_res(256, 224);
        window->change_window_res(256 * flags.zoom, 224 * flags.zoom);
        if (!flags.novideo)
            window->show();
        if (flags.fullscreen)
            window->set_fullscreen();
    }
#else
    (void)window;
#endif
    if (!flags.charmap.empty())
    {
        std::ifstream in(flags.charmap, std::ios::binary);
        if (!in)
            throw std::runtime_error("Cannot read texture file");
        load_textures(std::vector<uint8_t>(std::istreambuf_iterator<char>(in), {}));
    }
}
void AVC64::load_textures(const std::vector<uint8_t> &data)
{
    std::lock_guard lock(mutex);
    if (data.size() > 8192)
        throw std::runtime_error("Texture map exceeds 8192 bytes");
    std::map<uint16_t, Texture> loaded;
    for (size_t i = 0; i < data.size();)
    {
        if (data.size() - i < 6)
            throw std::runtime_error("Truncated texture header");
        unsigned id = (unsigned(data[i]) << 8) | data[i + 1], w = data[i + 2], h = data[i + 3], mode = data[i + 4];
        static const unsigned bits[] = {1, 2, 4, 8, 8, 12};
        if (!w || !h || mode > 5 || loaded.contains(id))
            throw std::runtime_error("Invalid texture header");
        size_t size = (w * h * bits[mode] + 7) / 8;
        if (size > data.size() - i - 5)
            throw std::runtime_error("Truncated texture data");
        loaded.emplace(id, Texture{w, h, mode, {data.begin() + i + 5, data.begin() + i + 5 + size}});
        i += 5 + size;
    }
    textures = std::move(loaded);
}
void AVC64::draw_texture()
{
    auto it = textures.find((unsigned(registers[2]) << 8) | registers[3]);
    if (it == textures.end())
        return;
    auto &texture = it->second;
    const unsigned bits[] = {1, 2, 4, 8, 8, 12};
    auto extract = [&](unsigned pixel)
    {
        unsigned result = 0;
        for (unsigned bit = 0; bit < bits[texture.mode]; ++bit)
        {
            unsigned position = pixel * bits[texture.mode] + bit;
            result = (result << 1) | ((texture.data[position / 8] >> (7 - position % 8)) & 1);
        }
        return result;
    };
    // Palette 16 is RGBI; palette 256 is RGB332. Alpha uses four bits.
    auto palette16 = [](unsigned c) -> uint8_t
    {
        unsigned low = c & 8 ? 3 : 0, high = c & 8 ? 7 : 4;
        return ((c & 4 ? high : low) << 5) | ((c & 2 ? high : low) << 2) | (c & 1 ? (high >> 1) : (low >> 1));
    };
    for (unsigned y = 0; y < texture.height; ++y)
        for (unsigned x = 0; x < texture.width; ++x)
        {
            unsigned dx = registers[0] + x, dy = registers[1] + y;
            if (dx >= 256 || dy >= 224)
                continue;
            unsigned value = extract(y * texture.width + x), alpha = 15;
            uint8_t color = 0;
            switch (texture.mode)
            {
            case 0:
                color = registers[value ? 4 : 5];
                break;
            case 1:
                alpha = value & 1 ? 15 : 0;
                color = registers[value & 2 ? 4 : 5];
                break;
            case 2:
                color = palette16(value);
                break;
            case 3:
                alpha = value & 15;
                color = palette16(value >> 4);
                break;
            case 4:
                color = value;
                break;
            case 5:
                alpha = value & 15;
                color = value >> 4;
                break;
            }
            auto &destination = pixels[dy * 256 + dx];
            if (alpha == 15)
                destination = color;
            else if (alpha)
            {
                auto blend = [&](unsigned shift, unsigned mask)
                {
                    return ((((color >> shift) & mask) * alpha + ((destination >> shift) & mask) * (15 - alpha) + 7) / 15) << shift;
                };
                destination = blend(5, 7) | blend(2, 7) | blend(0, 3);
            }
        }
    dirty = true;
}
void AVC64::cpu_write(uint16_t value, bool)
{
    std::lock_guard lock(mutex);
    unsigned op = value >> 8;
    if (op >= 3 && op <= 8)
    {
        registers[op - 3] = value;
        return;
    }
    switch (op)
    {
    case 0:
        if (registers[1] < 224)
            pixels[registers[1] * 256 + registers[0]] = registers[2];
        dirty = true;
        break;
    case 1:
        draw_texture();
        break;
    case 2:
        std::fill(pixels.begin(), pixels.end(), registers[2]);
        dirty = true;
        break;
    case 9:
        break;
    default:
        throw std::runtime_error("Unknown AVC64 command");
    }
}
uint16_t AVC64::cpu_read(uint16_t value, bool)
{
    std::lock_guard lock(mutex);
    unsigned op = value >> 8;
    if (op >= 3 && op <= 8)
        return registers[op - 3];
    if (op == 0 && registers[1] < 224)
        return pixels[registers[1] * 256 + registers[0]];
    return 0;
}
void AVC64::present()
{
#ifdef ANC216_WITH_SDL
    std::lock_guard lock(mutex);
    if (window && dirty)
    {
        window->present(pixels);
        dirty = false;
    }
#endif
}
