#pragma once

#include <stdint.h>
#include <types.hh>

#define MAX_MEM 65'536

namespace ANC216
{
    class EmemMapper
    {
    private:
        uint8_t emem[MAX_MEM];
    public:
        EmemMapper(const EmuFlags &flags)
        {

        }
    };
}