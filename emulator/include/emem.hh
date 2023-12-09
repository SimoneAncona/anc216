#pragma once

#include <stdint.h>
#include <types.hh>

#include <audio.hh>
#include <iostream>
#include <console.hh>
#include <device.hh>
#include <gpu.hh>
#include <tuple>

#define MAX_MEM 65'536

#define DEFAULT_VIDEO_CARD_ADDR 0xFFFD
#define DEFAULT_AUDIO_CARD_ADDR 0xFFFE

namespace ANC216
{
    class EmemMapper
    {
    private:
        std::pair<uint8_t, Device *> emem[MAX_MEM] = {{0, nullptr}};
        CPU *cpu;

    public:
        EmemMapper(const EmuFlags &flags)
        {
            this->cpu = cpu;
            if (!flags.novideo && flags.gpu == "")
            {
                std::cerr << RED << "emu::error " << RESET << "video enabled but no GPU specified. Try using --novideo or specifying the gpu with --gpu=" << std::endl;
                exit(EXIT_FAILURE);
            }
            if (flags.gpu == "default" || flags.gpu == "AVC64")
            {
                emem[DEFAULT_VIDEO_CARD_ADDR].second = new AVC64(this);
            }
        }

        inline void set_cpu(CPU *cpu)
        {
            this->cpu = cpu;
        }

        uint16_t where_am_i(Device* device)
        {
            for (uint16_t i = 0; i < MAX_MEM; i++)
                if (emem[i].second == device)
                    return i;
            return 0;
        }

        void write(uint16_t value, uint16_t address)
        {

        }

        void read(uint16_t value, uint16_t address)
        {

        }
    };
}