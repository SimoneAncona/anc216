#include <cpu.hh>
#include <emem.hh>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

static void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
static ANC216::CPUDebugState stopped(ANC216::CPU &cpu)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (cpu.debug_state().running && std::chrono::steady_clock::now() < deadline)
        std::this_thread::yield();
    auto state = cpu.debug_state();
    require(!state.running, "debugger run did not stop");
    return state;
}
int main()
{
    try
    {
        ANC216::EmuFlags flags;
        flags.novideo = true;
        flags.debug_mode = true;
        flags.uncapped = true;
        ANC216::EmemMapper mapper(flags);
        ANC216::CPU cpu(&mapper, flags);
        // INC R0; JMP ff00. A persistent breakpoint stops before INC.
        cpu.load({0x01, 0x2d, 0x80, 0x22, 0xff, 0x00});
        cpu.set_breakpoint(0xff00);
        cpu.start();
        auto state = stopped(cpu);
        require(state.breakpoint == 0xff00 && state.instructions == 0 && cpu.get_info().reg[0] == 0, "initial breakpoint must stop before execution");
        cpu.start();
        state = stopped(cpu);
        require(state.breakpoint == 0xff00 && state.instructions == 2 && cpu.get_info().reg[0] == 1, "continue must pass current breakpoint then catch next iteration");
        cpu.step();
        require(cpu.get_pc() == 0xff02 && cpu.get_info().reg[0] == 2, "single step must ignore a breakpoint");
        cpu.set_breakpoint(0xff00, false);
        cpu.step();
        cpu.run_until(0xff02);
        state = stopped(cpu);
        require(state.temporary && cpu.get_info().reg[0] == 3, "disabled breakpoint/temporary stop");
        cpu.remove_breakpoint(0xff00);
        require(cpu.list_breakpoints().empty(), "breakpoint removal");
        cpu.debug_set("r0", 0x1234);
        cpu.debug_set("l0", 0xab);
        require(uint16_t(cpu.get_info().reg[0]) == 0x12ab, "low-register editing must preserve the high byte");
        try
        {
            cpu.debug_set("sr", 0x100);
            throw std::runtime_error("invalid SR edit accepted");
        }
        catch (const std::invalid_argument &)
        {
        }
        // Nested callees preserve BP using PHBP/CALL/POBP. Step-over returns
        // only at the original stack position, rather than any matching PC.
        cpu.load_init_state();
        cpu.load({0x80, 4, 0xff, 6, 0, 0, 0, 0x0e, 0x80, 4, 0xff, 16, 0, 0x0f, 0, 5, 1, 0x2d, 0, 5});
        cpu.run_until(0xff04, 0x3000);
        state = stopped(cpu);
        require(state.temporary && cpu.get_pc() == 0xff04 && cpu.get_info().reg[0] == 1, "BP-preserving nested step-over");
        cpu.set_breakpoint(0xff06);
        cpu.load_init_state();
        require(cpu.list_breakpoints().contains(0xff06), "hard reset must preserve breakpoints");
        cpu.start();
        state = stopped(cpu);
        require(state.breakpoint == 0xff06 && cpu.get_info().sp == 0x3003, "callee entry breakpoint");
        cpu.clear_breakpoints();
        cpu.run_until(0xff04, 0x3000);
        state = stopped(cpu);
        require(state.temporary, "finish-style return stop");
        cpu.load_init_state();
        cpu.poke(2, 0x40);
        cpu.poke(3, 0);
        cpu.load({1, 0x2d, 0x80, 0x22, 0x40, 0}, 0x4000);
        cpu.set_breakpoint(0x4000);
        mapper.keyboard_input('A');
        cpu.start();
        state = stopped(cpu);
        require(state.breakpoint == 0x4000 && uint16_t(cpu.get_info().reg[0]) == DEFAULT_KEYBOARD_ADDR && state.instructions == 0, "IRQ breakpoint must stop before first handler instruction");
        std::cout << "Breakpoint, resume, step-over, finish, edit and IRQ debug checks passed\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
