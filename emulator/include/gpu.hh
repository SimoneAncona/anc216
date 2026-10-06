#pragma once
#include <device.hh>
#include <video.hh>
namespace ANC216
{
    class VideoCard : public Device
    {
    protected:
        Video::Window *window;

    public:
        VideoCard(EmemMapper *mapper, int width, int height, EmuFlags flags, Video::Window *win) : Device(mapper, flags), window(win)
        {
#ifdef ANC216_WITH_SDL
            if (!window || flags.novideo)
                return;
            window->init();
            window->change_window_res(width, height);
            if (!flags.novideo)
                window->show();
            if (flags.fullscreen)
                window->set_fullscreen();
#else
            (void)width;
            (void)height;
#endif
        }
    };
} // namespace ANC216
