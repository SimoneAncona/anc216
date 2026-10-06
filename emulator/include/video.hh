#include <functional>
#include <cstdint>
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
        SDL_Texture *texture = nullptr;
        std::vector<uint32_t> framebuffer;
        int width = 256, height = 224;
        bool closing = false;
        char last_key = 0;
        std::function<void(uint16_t)> input;
        std::function<void(bool)> control;

    public:
        Window() = default;
        bool close_requested() const
        {
            return closing;
        }
        void set_input_handlers(std::function<void(uint16_t)> key, std::function<void(bool)> pin)
        {
            input = std::move(key);
            control = std::move(pin);
        }
        ~Window()
        {
            if (texture)
                SDL_DestroyTexture(texture);
            if (renderer)
                SDL_DestroyRenderer(renderer);
            if (window)
                SDL_DestroyWindow(window);
            if (window)
            {
                SDL_StopTextInput();
                SDL_Quit();
            }
        }
        void init()
        {
            if (SDL_Init(SDL_INIT_VIDEO) != 0)
                throw std::runtime_error(SDL_GetError());
            window = SDL_CreateWindow("ANC216 AVC64", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 1120, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
            if (!window)
                throw std::runtime_error(SDL_GetError());
            SDL_StartTextInput();
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
        bool handle_event(const SDL_Event &event)
        {
            if (event.type == SDL_QUIT)
            {
                closing = true;
                if (!control)
                    return false;
                control(true);
            }
            if (event.type == SDL_KEYDOWN)
            {
                const auto key = event.key.keysym.sym;
                const bool ctrl = event.key.keysym.mod & KMOD_CTRL;
                if (ctrl && (key == SDLK_c || key == SDLK_d))
                {
                    if (!event.key.repeat && control)
                        control(key == SDLK_c);
                    return true;
                }
                uint16_t value = 0;
                if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
                    value = 10;
                else if (key == SDLK_BACKSPACE)
                    value = 8;
                else if (key == SDLK_TAB)
                    value = 9;
                else if (key == SDLK_ESCAPE)
                    value = 27;
                else if (key & SDLK_SCANCODE_MASK)
                    value = 0x8000 | event.key.keysym.scancode;
                if (value && input)
                    input(value);
            }
            if (event.type == SDL_TEXTINPUT && input)
                for (const unsigned char *p = reinterpret_cast<const unsigned char *>(event.text.text); *p; ++p)
                    if (*p >= 32 && *p < 127)
                    {
                        last_key = *p;
                        input(*p);
                    }
            return true;
        }
        bool poll()
        {
            if (!window)
                return true;
            SDL_Event event;
            while (SDL_PollEvent(&event))
                if (!handle_event(event))
                    return false;
            return true;
        }
        char key_pressed() const
        {
            return last_key;
        }
        void change_logical_res(int w, int h)
        {
            if (texture)
            {
                SDL_DestroyTexture(texture);
                texture = nullptr;
            }
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
            if (!texture)
            {
                texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                            SDL_TEXTUREACCESS_STREAMING, width, height);
                if (!texture)
                    throw std::runtime_error(SDL_GetError());
                SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
                framebuffer.resize(width * height);
            }
            // Upload a framebuffer rather than making 57,344 draw calls per frame.
            for (size_t i = 0; i < framebuffer.size(); ++i)
            {
                const uint32_t c = pixels[i];
                const uint32_t r = ((c >> 5) & 7) * 255 / 7;
                const uint32_t g = ((c >> 2) & 7) * 255 / 7;
                const uint32_t b = (c & 3) * 255 / 3;
                framebuffer[i] = 0xff000000 | (r << 16) | (g << 8) | b;
            }
            if (SDL_UpdateTexture(texture, nullptr, framebuffer.data(), width * sizeof(uint32_t)) != 0)
                throw std::runtime_error(SDL_GetError());
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            if (SDL_RenderCopy(renderer, texture, nullptr, nullptr) != 0)
                throw std::runtime_error(SDL_GetError());
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
        bool close_requested() const
        {
            return false;
        }
        void set_input_handlers(std::function<void(uint16_t)>, std::function<void(bool)>)
        {
        }
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
