#pragma once

#include <common.hh>
#include <video.hh>
#include <iostream>

class ANC216::VideoCard : public ANC216::Device
{
protected:
    ANC216::Video::Window *window;

public:
    VideoCard(ANC216::EmemMapper *emem, const int w_resolution, const int h_resolution, EmuFlags flags, Video::Window *win)
        : Device(emem, flags)
    {
        this->window = win;
        window->init();
        std::thread(
            [this, w_resolution, h_resolution, flags]
            { 
                this->window->wait_init(); 
                window->change_window_res(w_resolution, h_resolution);
                if (!flags.novideo)
                    this->window->show();
                if (flags.fullscreen) 
                    window->set_fullscreen(); 
            }
            ).join();
    }
};