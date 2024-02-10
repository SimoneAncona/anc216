#pragma once

#include <SDL.h>
#include <stdexcept>
#include <thread>
#include <iostream>

namespace ANC216::Video
{
    class Window
    {
    private:
        SDL_Window *window = NULL;
        SDL_Renderer *renderer = NULL;
        std::thread *thread;
        int r_width = 400, r_height = 400;
        char last_key = '\0';

        void _init()
        {
            if (SDL_Init(SDL_INIT_VIDEO) != 0)
                throw std::runtime_error("Cannot initialize video");
            window = SDL_CreateWindow("Emulated GPU video", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, r_width, r_height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
            if (window == NULL)
                throw std::runtime_error("Cannot create window");

            renderer = SDL_CreateRenderer(window, 0, 0);
            if (renderer == NULL)
                throw std::runtime_error("Cannot init renderer");

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
                case SDL_KEYDOWN:
                    last_key = event.key.keysym.sym;
                
                }
            }
        }

    public:
        Window() = default;
        ~Window()
        {
            delete thread;
        }

        inline void init()
        {
            thread = new std::thread([this]() { this->_init(); });
        }

        inline void wait_init()
        {
            while (renderer == NULL);
        }

        inline void wait()
        {
            if (thread != nullptr)
                thread->join();
        }

        char key_pressed()
        {
            return last_key;
        }

        inline void change_logical_res(const int width, const int height)
        {
            SDL_RenderSetLogicalSize(renderer, width, height);
        }

        inline void change_window_res(const int width, const int height)
        {
            SDL_SetWindowSize(window, width, height);
        }

        inline void set_fullscreen()
        {
            SDL_SetWindowFullscreen(window, 0);
        }

        inline void show()
        {
            SDL_ShowWindow(window);
        }

        inline void hide()
        {
            SDL_HideWindow(window);
        }

        inline SDL_Window *get_sdl_window()
        {
            return window;
        }

        inline SDL_Renderer *get_sdl_renderer()
        {
            return renderer;
        }
    };
}