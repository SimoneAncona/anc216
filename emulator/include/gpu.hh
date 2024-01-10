#pragma once

#include <common.hh>
#include <video.hh>

class ANC216::VideoCard : public ANC216::Device
{
protected:
    ANC216::Video::Window window;
public:
    VideoCard(ANC216::EmemMapper *emem, const int w_resolution, const int h_resolution) 
        : Device(emem), window(w_resolution, h_resolution)
    {
    }
};