#include <avc64.hh>
#include <video.hh>

ANC216::AVC64::AVC64(EmemMapper *emem, const int w_res, const int h_res) : ANC216::VideoCard(emem, w_res, h_res)
{
    this->id = ANC216::AVC64_VIDEO_CARD;
    
}

void ANC216::AVC64::cpu_write(uint16_t value, bool additional_flag)
{
}

uint16_t ANC216::AVC64::cpu_read(uint16_t value, bool additional_flag)
{
    return 0;
}
