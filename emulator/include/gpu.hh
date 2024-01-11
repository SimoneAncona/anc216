#pragma once

#include <common.hh>
#include <video.hh>

class ANC216::VideoCard : public ANC216::Device
{
protected:
    ANC216::Video::Window *window;
public:
    VideoCard(ANC216::EmemMapper *emem, const int w_resolution, const int h_resolution, EmuFlags flags, ANC216::Video::Window *win) 
        : Device(emem, flags)
    {
        this->window = win;
        if (!flags.novideo)
            this->window->show();
        if (flags.fullscreen) window->set_fullscreen();
    }
};