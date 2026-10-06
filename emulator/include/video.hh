#pragma once
#ifdef ANC216_WITH_SDL
#include <SDL.h>
#include <stdexcept>
#include <cstdint>
#include <vector>
namespace ANC216::Video
{
    class Window
    {
        SDL_Window *window = nullptr;
        SDL_Renderer *renderer = nullptr;
        int width = 256, height = 224;
        char last_key = 0;

    public:
        Window() = default;
        ~Window()
        {
            if (renderer)
                SDL_DestroyRenderer(renderer);
            if (window)
                SDL_DestroyWindow(window);
            if (window)
                SDL_Quit();
        }
        void init()
        {
            if (SDL_Init(SDL_INIT_VIDEO) != 0)
                throw std::runtime_error(SDL_GetError());
            window = SDL_CreateWindow("ANC216 AVC64", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 512, 448, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
            if (!window)
                throw std::runtime_error(SDL_GetError());
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
            if (!renderer)
                throw std::runtime_error(SDL_GetError());
        }
        void wait_init()
        {
        }
        void wait()
        {
        }
        bool poll()
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT)
                    return false;
                if (event.type == SDL_KEYDOWN)
                    last_key = event.key.keysym.sym;
            }
            return true;
        }
        char key_pressed() const
        {
            return last_key;
        }
        void change_logical_res(int w, int h)
        {
            width = w;
            height = h;
            SDL_RenderSetLogicalSize(renderer, w, h);
        }
        void change_window_res(int w, int h)
        {
            SDL_SetWindowSize(window, w, h);
        }
        void set_fullscreen()
        {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        }
        void show()
        {
            SDL_ShowWindow(window);
        }
        void hide()
        {
            SDL_HideWindow(window);
        }
        SDL_Window *get_sdl_window()
        {
            return window;
        }
        SDL_Renderer *get_sdl_renderer()
        {
            return renderer;
        }
        void present(const std::vector<uint8_t> &pixels)
        {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                {
                    const uint8_t c = pixels[y * width + x];
                    SDL_SetRenderDrawColor(renderer, ((c >> 5) & 7) * 255 / 7, ((c >> 2) & 7) * 255 / 7, (c & 3) * 255 / 3, 255);
                    SDL_RenderDrawPoint(renderer, x, y);
                }
            SDL_RenderPresent(renderer);
        }
    };
} // namespace ANC216::Video
#else
namespace ANC216::Video
{
    class Window
    {
    public:
        void wait()
        {
        }
        bool poll()
        {
            return true;
        }
        char key_pressed() const
        {
            return 0;
        }
    };
} // namespace ANC216::Video
#endif
