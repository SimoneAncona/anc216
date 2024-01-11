#pragma once

#include <SDL.h>
#include <stdexcept>
#include <thread>

namespace ANC216::Video
{
    class Window
    {
    private:
        SDL_Window *window;
        SDL_Renderer *renderer;
        int r_width, r_height;

        void init()
        {
            if (SDL_Init(SDL_INIT_VIDEO) != 0)
                throw std::runtime_error("Cannot initialize video");
            window = SDL_CreateWindow("Emulated GPU video", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, r_width, r_height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
            if (window == NULL)
                throw std::runtime_error("Cannot create window");
            auto run = true;
            SDL_Event event;

            while (run)
            {
                SDL_PollEvent(&event);

                switch (event.type)
                {
                case SDL_QUIT:
                    run = false;
                    SDL_Quit();
                    exit(0);
                    break;
                }
            }
        }

    public:
        Window()
        {
            std::thread thread([this]() { this->init(); });
            thread.join();
        }

        void change_logical_res(const int width, const int height)
        {
            SDL_RenderSetLogicalSize(renderer, width, height);
        }

        void set_fullscreen()
        {
            SDL_SetWindowFullscreen(window, 0);
        }

        void show()
        {
            SDL_ShowWindow(window);
        }

        void hide()
        {
            SDL_HideWindow(window);
        }
    };
}