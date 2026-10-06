#pragma once
#include <cstdint>
#include <types.hh>
#define MAX_MEM 65'536
#define DEFAULT_VIDEO_CARD_ADDR 0xFFFD
#define DEFAULT_KEYBOARD_ADDR 0xFFFC
#define DEFAULT_AUDIO_CARD_ADDR 0xFFFE
#define ROM_ADDR 0xFF00
namespace ANC216
{
    enum DeviceID
    {
        ROM = 0,
        KEYBOARD = 0x0301,
        MPME_CARD = 0x0102,
        AVC64_VIDEO_CARD = 0x0201,
        GENERIC_VIDEO_CARD = 0x02FF,
        AUDIO_CARD = 0x0200
    };
    class Device;
    class EmemMapper;
    class CPU;
    class VideoCard;
    class AVC64;
    struct CPUInfo;
} // namespace ANC216
