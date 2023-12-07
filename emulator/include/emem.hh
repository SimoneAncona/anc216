#pragma once

#include <stdint.h>

#define MAX_MEM 65'536

namespace ANC216
{
    class EmemMapper
    {
    private:
        uint8_t emem[MAX_MEM];
    };
}