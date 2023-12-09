#include <string>
#include <vector>
#include <tuple>
#include <stdint.h>

#pragma once

namespace ANC216
{
    struct EmuFlags
    {
        char debug_mode : 1 = 0;
        char fast_mode : 1 = 0;
        char noaudio : 1 = 0;
        char novideo : 1 = 0;
        char nokeyboard : 1 = 0;
        std::string gpu;
        std::vector<std::pair<uint16_t, std::string>> extensions;
        std::vector<std::pair<uint16_t, std::string>> inserts;
        std::vector<std::pair<uint16_t, std::string>> cards;
        std::string charmap;
        float speed = 1;
    };
}