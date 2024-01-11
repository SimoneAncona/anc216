#include <avc64.hh>
#include <video.hh>

#define W_AVC64_RES 256
#define H_AVC64_RES 254

ANC216::AVC64::AVC64(EmemMapper *emem, EmuFlags flags, Video::Window *win) : ANC216::VideoCard(emem, W_AVC64_RES * 2, H_AVC64_RES * 2, flags, win)
{
    this->id = ANC216::AVC64_VIDEO_CARD;
    this->window->change_logical_res(W_AVC64_RES, H_AVC64_RES);
}

void ANC216::AVC64::cpu_write(uint16_t value, bool additional_flag)
{
}

uint16_t ANC216::AVC64::cpu_read(uint16_t value, bool additional_flag)
{
    return 0;
}
