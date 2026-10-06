#include <cpu.hh>
#include <emem.hh>
#include <video.hh>
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
int main()
{
    try
    {
        ANC216::EmuFlags flags;
        flags.novideo = true;
        flags.debug_mode = true;
        ANC216::EmemMapper mapper(flags);
        ANC216::CPU cpu(&mapper, flags);
        require(mapper.info_req(DEFAULT_KEYBOARD_ADDR) == ANC216::KEYBOARD, "keyboard identification");
        cpu.load({0, 0x13, 0, 0x22, 0xff, 0}, ROM_ADDR);
        mapper.keyboard_input('A');
        cpu.step();
        require(mapper.read(1, DEFAULT_KEYBOARD_ADDR) == 1, "masked keyboard must remain queued");
        require(mapper.read(0, DEFAULT_KEYBOARD_ADDR) == 'A', "polling keyboard data");
        for (unsigned i = 0; i < 65; ++i)
            mapper.keyboard_input('B');
        require(mapper.read(1, DEFAULT_KEYBOARD_ADDR) == 64 && mapper.read(2, DEFAULT_KEYBOARD_ADDR) == 1, "bounded queue/overflow counter");
        mapper.write(0, DEFAULT_KEYBOARD_ADDR);
        cpu.load_init_state();
        cpu.poke(2, 0x40);
        cpu.poke(3, 0);
        cpu.load({0x80, 0x22, 0x40, 0}, 0x4000);
        mapper.keyboard_input('C');
        cpu.step();
        auto info = cpu.get_info();
        require(info.pc == 0x4000 && info.sp == 0x3008 && uint16_t(info.reg[0]) == DEFAULT_KEYBOARD_ADDR &&
                    uint16_t(info.reg[1]) == 'C' && uint8_t(info.reg[2]) == 1,
                "keyboard IRQ register/frame ABI");
        require(mapper.read(1, DEFAULT_KEYBOARD_ADDR) == 0, "delivered event must be consumed once");
        cpu.load_init_state();
        cpu.poke(4, 0x40);
        cpu.poke(5, 0);
        cpu.request_soft_reset();
        cpu.step();
        info = cpu.get_info();
        require(info.pc == 0x4000 && uint16_t(info.reg[0]) == 5 && info.sp == 0x3004, "soft reset NMI");
        cpu.poke(10, 0x50);
        cpu.poke(11, 0);
        cpu.load({0x80, 0x22, 0x50, 0}, 0x5000);
        cpu.request_shutdown();
        cpu.step();
        require(cpu.get_pc() == 0x5000 && cpu.get_info().sp == info.sp, "shutdown pin must not push an interrupt frame");
#ifdef ANC216_WITH_SDL
        ANC216::Video::Window window;
        window.init();
        int width = 0, height = 0;
        SDL_GetWindowSize(window.get_sdl_window(), &width, &height);
        require(width == 1280 && height == 1120, "default window dimensions");
        window.present(std::vector<uint8_t>(256 * 224, 0xff));
        window.present(std::vector<uint8_t>(256 * 224, 0x00));
        window.change_window_res(256 * 2, 224 * 2);
        SDL_GetWindowSize(window.get_sdl_window(), &width, &height);
        require(width == 512 && height == 448, "zoom window dimensions");
        bool reset = false, shutdown = false;
        uint16_t key = 0;
        window.set_input_handlers([&](uint16_t value)
                                  {
                                      key = value;
                                  },
                                  [&](bool value)
                                  {
                                      shutdown |= value;
                                      reset |= !value;
                                  });
        SDL_Event event{};
        event.type = SDL_KEYDOWN;
        event.key.keysym.sym = SDLK_d;
        event.key.keysym.mod = KMOD_CTRL;
        SDL_PushEvent(&event);
        event.key.keysym.sym = SDLK_c;
        SDL_PushEvent(&event);
        event = {};
        event.type = SDL_TEXTINPUT;
        event.text.windowID = SDL_GetWindowID(window.get_sdl_window());
        event.text.text[0] = 'X';
        window.handle_event(event);
        require(window.poll() && reset && shutdown && key == 'X', "SDL controls/text input");
#endif
        std::cout << "Keyboard polling, queue, IRQ, reset/shutdown and SDL controls passed\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
