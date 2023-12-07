#include <stdint.h>
#include <emem.hh>
#include <types.hh>

#pragma once

#ifndef MAX_MEM
    #define MAX_MEM 65'536
#endif

namespace ANC216
{
    class Emu
    {
    private:
        int16_t r0;
        int16_t r1;
        int16_t r2;
        int16_t r3;
        int16_t r4;
        int16_t r5;
        int16_t r6;
        int16_t r7;

        uint8_t sr;
        uint16_t sp;
        uint16_t bp;

        uint16_t pc;
        uint16_t current_instruction;

        uint8_t imem[MAX_MEM];
        EmemMapper &emem;

        inline int8_t get_lower(int16_t val)
        {
            return val & 0xFF;
        }
    public:
        Emu(EmemMapper &mapper, EmuFlags &flags): emem(mapper) {}
    };
}