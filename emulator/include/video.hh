#pragma once

#include <SDL.h>
#include <stdexcept>
#include <future>

namespace ANC216::Video
{
    class Window
    {
    private:
        SDL_Window *window;
        SDL_Renderer *renderer;
        int r_width = 100, r_height = 100;

        void _init()
        {
            if (SDL_Init(SDL_INIT_VIDEO) != 0)
                throw std::runtime_error("Cannot initialize video");
            window = SDL_CreateWindow("Emulated GPU video", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, r_width, r_height, SDL_WINDOW_RESIZABLE);
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
        Window() = default;

        void init()
        {
            //std::async loop([this]() { this->init(); });
            
        }

        void change_logical_res(const int width, const int height)
        {
            SDL_RenderSetLogicalSize(renderer, width, height);
        }

        void change_window_res(const int width, const int height)
        {
            SDL_SetWindowSize(window, width, height);
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