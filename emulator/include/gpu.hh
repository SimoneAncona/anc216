#pragma once

#include <common.hh>
#include <video.hh>
#include <iostream>

class ANC216::VideoCard : public ANC216::Device
{
protected:
    ANC216::Video::Window window;
public:
    VideoCard(ANC216::EmemMapper *emem, const int w_resolution, const int h_resolution, EmuFlags flags) 
        : Device(emem, flags)
    {
        std::cout << "1" << std::endl;
        window.init();
        std::cout << "2" << std::endl;
        window.change_window_res(w_resolution, h_resolution);
        // if (!flags.novideo)
            this->window.show();
        if (flags.fullscreen) window.set_fullscreen();
    }
};