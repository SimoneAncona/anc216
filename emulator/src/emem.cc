#include <emem.hh>
#include <avc64.hh>
#include <iostream>

ANC216::EmemMapper::EmemMapper(const EmuFlags &flags, Video::Window *window)
{
    this->cpu = cpu;
    if (!flags.novideo && flags.gpu == "")
    {
        std::cerr << RED << "emu::error " << RESET << "video enabled but no GPU specified. Try using --novideo or specifying the gpu with --gpu=" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (flags.gpu == "default" || flags.gpu == "AVC64")
    {
        emem[DEFAULT_VIDEO_CARD_ADDR].second = new AVC64(this, flags, window);
    }
}


ANC216::EmemMapper::~EmemMapper()
{
    delete this->emem;
}

void ANC216::EmemMapper::set_cpu(ANC216::CPU *cpu)
{
    this->cpu = cpu;
}

uint16_t ANC216::EmemMapper::where_am_i(const ANC216::Device *device)
{
    for (uint16_t i = 0; i < MAX_MEM; i++)
        if (emem[i].second == device)
            return i;
    return 0;
}

void ANC216::EmemMapper::write(uint16_t value, uint16_t address)
{
}

void ANC216::EmemMapper::read(uint16_t value, uint16_t address)
{
}

void ANC216::EmemMapper::info_req(uint16_t address)
{
}
