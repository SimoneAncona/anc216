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

        void handle_events()
        {
            while (true)
            {
                SDL_Event event;
                SDL_PollEvent(&event);

                switch (event.type)
                {
                case SDL_QUIT:
                    SDL_Quit();
                    exit(0);
                    break;
                }
            }
        }

    public:
        Window(const int width, const int height)
        {
            if (SDL_Init(SDL_INIT_VIDEO) != 0)
                throw std::runtime_error("Cannot initialize video");
            window = SDL_CreateWindow("Emulated GPU video", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
            if (window == NULL)
                throw std::runtime_error("Cannot create window");

            std::thread thread([this]() { this->handle_events(); });
            thread.join();
        }
    };
}