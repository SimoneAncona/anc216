#include <cpu.hh>
#include <pacing.hh>
#include <emem.hh>
#include <isa.hh>
#include "../../common/encoding.hh"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace
{
    constexpr uint8_t N = 0x80, O = 0x40, I = 0x20, T = 0x10, S = 8, A = 4, Z = 2, C = 1;
    struct Fault
    {
        int code;
    };
} // namespace
using namespace ANC216;

CPU::CPU(EmemMapper *mapper, const EmuFlags &options) : emem(mapper), flags(options)
{
    load_init_state();
    if (!flags.bootfile.empty())
    {
        std::ifstream in(flags.bootfile, std::ios::binary);
        if (!in)
            throw std::runtime_error("Cannot open boot image: " + flags.bootfile);
        load(std::vector<uint8_t>(std::istreambuf_iterator<char>(in), {}));
    }
    if (emem)
        emem->set_cpu(this);
    // Start explicitly after all devices have been attached.
}
CPU::~CPU()
{
    shutdown();
    wait();
    if (emem)
        emem->set_cpu(nullptr);
}
void CPU::load_init_state()
{
    shutdown();
    wait();
    std::lock_guard lock(mutex);
    reg.fill(0);
    sr = 0x3c;
    pc = ROM_ADDR;
    sp = bp = 0x3000;
    current_instruction = 0;
    killed = false;
    running = false;
    failure.clear();
    breakpoint_hit.reset();
    resume_breakpoint.reset();
    temporary_pc.reset();
    temporary_sp.reset();
    temporary_hit = false;
    instruction_count = 0;
    mtu = {0, 0xfeff, 0, 0xffff, 0x3200, 0xfeff};
    cpuid = 0x8000;
    timer = 0;
    timer_running = false;
    reset_requested = shutdown_requested = false;
    // Default system stack pointer; guest firmware may replace it.
    imem[0x0c] = 0x30;
    imem[0x0d] = 0;
}
void CPU::load(const std::vector<uint8_t> &data, uint16_t address)
{
    std::lock_guard lock(mutex);
    if (data.size() > MAX_MEM - unsigned(address))
        throw std::runtime_error("Image exceeds available memory (boot ROM is 256 bytes)");
    std::copy(data.begin(), data.end(), imem.begin() + address);
}
void CPU::start()
{
    std::lock_guard lock(mutex);
    if (killed)
        return;
    if (!running)
    {
        resume_breakpoint = breakpoint_hit;
        breakpoint_hit.reset();
        temporary_hit = false;
    }
    running = true;
    if (!worker.joinable())
        worker = std::thread([this]
                             {
                                 cycle();
                             });
    wake.notify_all();
}
void CPU::stop()
{
    std::lock_guard lock(mutex);
    running = false;
    temporary_pc.reset();
    temporary_sp.reset();
}
void CPU::shutdown()
{
    std::lock_guard lock(mutex);
    killed = true;
    running = false;
    wake.notify_all();
}
void CPU::wait()
{
    if (worker.joinable())
        worker.join();
}
void CPU::cycle()
{
    ExecutionPacer pacer(flags);
    std::unique_lock lock(mutex);
    while (!killed)
    {
        wake.wait(lock, [this]
                  {
                      return running || killed;
                  });
        if (killed)
            break;
        for (unsigned i = 0; i < pacer.batch && running && !killed; ++i)
            execute(true);
        if (!killed && running)
        {
            if (pacer.uncapped())
            {
                // yield() can immediately reacquire this unfair mutex and
                // starve debugger pause/snapshot requests. A short wait releases
                // it while giving the console a scheduling opportunity.
                wake.wait_for(lock, std::chrono::microseconds(1), [this]
                              {
                                  return killed || !running;
                              });
            }
            else
                wake.wait_until(lock, pacer.deadline(), [this]
                                {
                                    return killed || !running;
                                });
        }
    }
}
void CPU::step()
{
    std::lock_guard lock(mutex);
    running = false;
    breakpoint_hit.reset();
    temporary_pc.reset();
    temporary_sp.reset();
    temporary_hit = false;
    resume_breakpoint.reset();
    if (!killed)
        execute();
}
uint16_t CPU::get_pc()
{
    std::lock_guard lock(mutex);
    return pc;
}
uint16_t CPU::get_current_instruction()
{
    std::lock_guard lock(mutex);
    return current_instruction;
}
CPUInfo CPU::get_info()
{
    std::lock_guard lock(mutex);
    return {reg, mtu, sr, sp, bp, pc, current_instruction};
}
uint8_t CPU::peek(uint16_t a) const
{
    std::lock_guard lock(mutex);
    return imem[a];
}
void CPU::poke(uint16_t a, uint8_t v)
{
    std::lock_guard lock(mutex);
    imem[a] = v;
}
bool CPU::halted() const
{
    std::lock_guard lock(mutex);
    return killed;
}
std::string CPU::error() const
{
    std::lock_guard lock(mutex);
    return failure;
}
uint16_t CPU::mapped(uint16_t a, bool external) const
{
    if (sr & S)
        return a;
    if (external)
    {
        // EMEM operands always name physical device addresses, including user IO.
        if (a < mtu[2] || a > mtu[3])
            throw Fault{3};
        return a;
    }
    unsigned index = 0;
    unsigned physical = unsigned(mtu[index]) + a;
    if (physical > mtu[index + 1] || physical >= MAX_MEM)
        throw Fault{external ? 3 : 2};
    return physical;
}
void CPU::check(uint16_t a, unsigned width, bool writing) const
{
    if (unsigned(a) + width > MAX_MEM)
        throw Fault{2};
    if (writing && unsigned(a) + width > ROM_ADDR)
        throw Fault{2};
    if (!(sr & S) && (a < mtu[0] || unsigned(a) + width - 1 > mtu[1]))
        throw Fault{2};
}
uint16_t CPU::read(uint16_t a, unsigned width) const
{
    check(a, width);
    return width == 1 ? imem[a] : (unsigned(imem[a]) << 8) | imem[a + 1];
}
void CPU::write(uint16_t a, uint16_t value, unsigned width)
{
    check(a, width, true);
    if (width == 1)
        imem[a] = value;
    else
    {
        imem[a] = value >> 8;
        imem[a + 1] = value;
    }
}
void CPU::push(uint16_t value, unsigned width)
{
    if (unsigned(sp) + width > ROM_ADDR || (!(sr & S) && (sp < mtu[4] || unsigned(sp) + width - 1 > mtu[5])))
        throw Fault{4};
    write(sp, value, width);
    sp += width;
}
uint16_t CPU::pop(unsigned width)
{
    if (sp < width || (!(sr & S) && (unsigned(sp) - width < mtu[4] || sp - 1 > mtu[5])))
        throw Fault{4};
    uint16_t value = read(sp - width, width);
    sp -= width;
    return value;
}
void CPU::nz(uint16_t value, unsigned width)
{
    unsigned mask = width == 1 ? 0xff : 0xffff, sign = width == 1 ? 0x80 : 0x8000;
    sr = (sr & ~(N | Z)) | ((value & mask) == 0 ? Z : 0) | (value & sign ? N : 0);
}
void CPU::set_register(unsigned r, uint16_t value, unsigned width)
{
    reg[r] = width == 1 ? (uint16_t(reg[r]) & 0xff00) | (value & 0xff) : value;
}
void CPU::interrupt(uint16_t vector, int code)
{
    // Read protected vectors before switching to the kernel privilege level.
    uint16_t target = (unsigned(imem[vector]) << 8) | imem[vector + 1];
    if (!target)
    {
        failure = "Unhandled interrupt at vector " + std::to_string(vector) + " (code " + std::to_string(code) + ")";
        killed = true;
        running = false;
        return;
    }
    uint8_t saved_sr = sr;
    uint16_t saved_pc = pc;
    imem[0x31fe] = sp >> 8;
    imem[0x31ff] = sp;
    // Preserve BP alongside the documented SP save slot for manual restoration.
    imem[0x31fc] = bp >> 8;
    imem[0x31fd] = bp;
    sr = (sr | S) & ~(I | T);
    bp = 0x3000;
    sp = (unsigned(imem[0x0c]) << 8) | imem[0x0d];
    if (sp < 0x3000 || sp > 0x31f0)
        throw std::runtime_error("Invalid system stack pointer");
    push(saved_pc);
    push(saved_sr, 1);
    if (code >= 0)
    {
        push(uint8_t(reg[0]), 1);
        reg[0] = code;
    }
    pc = target;
}
bool CPU::einr(uint16_t address, uint16_t data, uint8_t type)
{
    std::lock_guard lock(mutex);
    if (!(sr & I) || killed)
        return false;
    try
    {
        auto saved = reg;
        interrupt(2);
        if (killed)
            return false;
        push(saved[0]);
        push(saved[1]);
        push(uint8_t(saved[2]), 1);
        reg[0] = address;
        reg[1] = data;
        set_register(2, type, 1);
        return true;
    }
    catch (const std::exception &e)
    {
        failure = e.what();
        killed = true;
    }
    return false;
}
void CPU::update_timer()
{
    if (!timer_running)
        return;
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - timer_tick).count();
    if (elapsed <= 0)
        return;
    timer_tick += std::chrono::milliseconds(elapsed);
    if (elapsed >= timer)
    {
        timer = 0;
        timer_running = false;
        if (sr & T)
            interrupt(8);
    }
    else
        timer -= elapsed;
}
void CPU::execute(bool honor_breakpoints)
{
    using namespace anc216_isa;
    uint16_t instruction_pc = pc;
    try
    {
        if (shutdown_requested)
        {
            shutdown_requested = false;
            reset_requested = false;
            const uint16_t target = (unsigned(imem[10]) << 8) | imem[11];
            if (!target)
            {
                killed = true;
                running = false;
                return;
            }
            // Shutdown pin uses no interrupt frame in the architecture PDF.
            sr = (sr | S) & ~(I | T);
            pc = target;
        }
        else if (reset_requested)
        {
            reset_requested = false;
            interrupt(4, 5);
        }
        if (emem && !killed)
            emem->pump_keyboard();
        update_timer();
        if (killed)
            return;
        instruction_pc = pc;
        // Check after pending interrupts choose their handler PC, before fetch.
        // A breakpoint on a vector target must stop before its first instruction.
        if (honor_breakpoints)
        {
            const auto found = breakpoints.find(pc);
            const bool persistent = found != breakpoints.end() && found->second && resume_breakpoint != pc;
            const bool temporary = temporary_pc == pc && (!temporary_sp || *temporary_sp == sp);
            if (persistent || temporary)
            {
                running = false;
                breakpoint_hit = pc;
                temporary_hit = temporary;
                ++stop_serial;
                if (temporary)
                {
                    temporary_pc.reset();
                    temporary_sp.reset();
                }
                return;
            }
        }
        resume_breakpoint.reset();
        ++instruction_count;
        check(pc, 2);
        const uint8_t addressing = imem[pc], op = imem[pc + 1];
        current_instruction = (unsigned(addressing) << 8) | op;
        const auto e = decode(addressing);
        if (!valid(op, e))
            throw Fault{0};
        if (opcode(op).privileged && !(sr & S))
            throw Fault{1};
        check(pc, 2 + e.size);
        const unsigned operands = unsigned(pc) + 2;
        pc = uint16_t(unsigned(pc) + 2 + e.size);
        auto byte = [&](unsigned at)
        {
            return uint16_t(imem[at]);
        };
        auto word = [&](unsigned at)
        {
            return uint16_t((unsigned(imem[at]) << 8) | imem[at + 1]);
        };
        auto immediate = [&](unsigned at)
        {
            return e.width == 1 ? byte(at) : word(at);
        };
        uint16_t address = 0, data = 0;
        bool has_address = false;
        const int index = static_cast<int8_t>(reg[e.reg] & 0xff);
        // IO absolute addresses refer to EMEM, other absolute addresses to IMEM.
        const bool io = op >= IREQ && op <= READ;
        switch (e.mode)
        {
        case Mode::immediate:
            data = immediate(operands);
            break;
        case Mode::reg:
            data = e.width == 1 ? uint8_t(reg[e.reg]) : uint16_t(reg[e.reg]);
            break;
        case Mode::regs:
            data = reg[e.source];
            break;
        case Mode::absolute:
        case Mode::reg_absolute:
        case Mode::store_absolute:
            address = mapped(word(operands), io);
            has_address = true;
            break;
        case Mode::indexed:
        case Mode::store_indexed:
            address = mapped(uint16_t(word(operands) + index), io);
            has_address = true;
            break;
        case Mode::indirect:
        case Mode::indirect_indexed:
            address = mapped(uint16_t(read(mapped(word(operands))) + (e.mode == Mode::indirect_indexed ? index : 0)), io);
            has_address = true;
            break;
        case Mode::pc:
        case Mode::reg_pc:
            address = uint16_t(pc + static_cast<int8_t>(byte(operands)));
            has_address = true;
            break;
        case Mode::bp:
        case Mode::reg_bp:
        case Mode::store_bp:
            address = uint16_t(bp + static_cast<int8_t>(byte(operands)));
            has_address = true;
            break;
        case Mode::pc_reg:
            address = uint16_t(pc + index);
            has_address = true;
            break;
        case Mode::bp_reg:
        case Mode::store_bp_reg:
            address = uint16_t(bp + index);
            has_address = true;
            break;
        case Mode::reg_immediate:
            data = immediate(operands);
            break;
        default:
            break;
        }
        if (e.mode == Mode::store_absolute || e.mode == Mode::store_indexed)
            data = immediate(operands + 2);
        else if (e.mode == Mode::store_bp)
            data = immediate(operands + 1);
        else if (e.mode == Mode::store_bp_reg)
            data = immediate(operands);
        else if (op == STORE || op == WRITE || op == HWRITE)
            data = e.width == 1 ? uint8_t(reg[e.reg]) : uint16_t(reg[e.reg]);
        else if (has_address && !(op >= JMP && op <= JNN) && op != CALL && !io && !(op >= STSR && op <= STBP))
            data = read(address, op == LDSR ? 1 : e.width);
        if (io)
        {
            // Validate every addressing family, including register and relative IO.
            // IO devices use one selector; flat ROM words span two EMEM cells.
            address = mapped(has_address ? address : data, true);
            if (!(sr & S) && emem && emem->is_rom(address) &&
                unsigned(address) + e.width - 1 > mtu[3])
                throw Fault{3};
            has_address = true;
        }
        auto assign = [&](uint16_t v)
        {
            set_register(e.reg, v, e.width);
            nz(v, e.width);
        };
        auto arith = [&](uint16_t left, uint16_t right, bool subtract)
        {
            const unsigned mask = e.width == 1 ? 0xff : 0xffff, sign = e.width == 1 ? 0x80 : 0x8000;
            left &= mask;
            right &= mask;
            const uint32_t result = subtract ? uint32_t(left) - right : uint32_t(left) + right;
            const uint16_t value = result & mask;
            const bool overflow = subtract ? ((left ^ right) & (left ^ value) & sign) : ((~(left ^ right)) & (left ^ value) & sign);
            sr = (sr & ~(O | C)) | (overflow ? O : 0) | ((subtract ? left < right : result > mask) ? C : 0);
            nz(value, e.width);
            return value;
        };
        const uint16_t left = e.width == 1 ? uint8_t(reg[e.reg]) : uint16_t(reg[e.reg]);
        const bool negative = sr & N, overflow = sr & O, zero = sr & Z;
        switch (op)
        {
        case KILL:
            killed = true;
            running = false;
            break;
        case RESETI:
            interrupt(4, 5);
            break;
        case CPUID:
            reg[0] = cpuid;
            break;
        case SYSCALL:
            interrupt(6);
            break;
        case CALL:
            push(pc);
            push(sr, 1);
            bp = sp;
            pc = address;
            break;
        case RET:
        {
            if (bp < 3)
                throw Fault{4};
            // BP marks the first local byte, immediately after PC/SR.
            const uint8_t saved = read(bp - 1, 1);
            const uint16_t target = read(bp - 3);
            sp = bp - 3;
            pc = target;
            if (!(sr & S))
                sr = (saved & ~(S | I | T)) | (sr & (S | I | T));
            else
                sr = saved;
            break;
        }
        case PUSH:
            push(data, e.width);
            break;
        case POP:
            assign(pop(e.width));
            break;
        case PHPC:
            push(pc);
            break;
        case POPC:
            pc = pop();
            break;
        case PHSR:
            push(sr, 1);
            break;
        case POSR:
        {
            uint8_t saved = pop(1);
            if (!(sr & S))
                sr = (saved & ~(S | I | T)) | (sr & (S | I | T));
            else
                sr = saved;
            break;
        }
        case PHSP:
            push(sp);
            break;
        case POSP:
            sp = pop();
            break;
        case PHBP:
            push(bp);
            break;
        case POBP:
            bp = pop();
            nz(bp, 2);
            break;
        case SETI:
            sr |= I;
            break;
        case SETT:
            sr |= T;
            break;
        case SETS:
            sr |= S;
            break;
        case CLRI:
            sr &= ~I;
            break;
        case CLRT:
            sr &= ~T;
            break;
        case CLRS:
            sr &= ~S;
            break;
        case CLRN:
            sr &= ~N;
            break;
        case CLRO:
            sr &= ~O;
            break;
        case CLRC:
            sr &= ~C;
            break;
        case PAREQ:
            sr |= A;
            break;
        case CAREQ:
            sr &= ~A;
            break;
        case IREQ:
            if (emem)
            {
                reg[0] = emem->info_req(has_address ? address : data);
                reg[1] = has_address ? address : data;
            }
            break;
        case REQ:
        case HREQ:
            if (emem)
                emem->request(has_address ? address : data, uint16_t(reg[1]), sr & A, op == HREQ);
            break;
        case READ:
            reg[1] = emem ? emem->read(uint16_t(reg[1]), has_address ? address : data, sr & A, e.width) : 0;
            break;
        case WRITE:
        case HWRITE:
            if (emem)
                emem->write(data, address, sr & A, e.width, op == HWRITE);
            break;
        case CMP:
            arith(left, data, true);
            break;
        case JMP:
            pc = address;
            break;
        case JEQ:
            if (zero)
                pc = address;
            break;
        case JNE:
            if (!zero)
                pc = address;
            break;
        case JGE:
            if (negative == overflow)
                pc = address;
            break;
        case JGR:
            if (negative == overflow && !zero)
                pc = address;
            break;
        case JLE:
            if (negative != overflow || zero)
                pc = address;
            break;
        case JLS:
            if (negative != overflow)
                pc = address;
            break;
        case JO:
            if (overflow)
                pc = address;
            break;
        case JNO:
            if (!overflow)
                pc = address;
            break;
        case JN:
            if (negative)
                pc = address;
            break;
        case JNN:
            if (!negative)
                pc = address;
            break;
        case INC:
            set_register(e.reg, arith(left, 1, false), e.width);
            break;
        case DEC:
            set_register(e.reg, arith(left, 1, true), e.width);
            break;
        case ADD:
            set_register(e.reg, arith(left, data, false), e.width);
            break;
        case SUB:
            set_register(e.reg, arith(left, data, true), e.width);
            break;
        case NEG:
            set_register(e.reg, arith(0, left, true), e.width);
            break;
        case AND:
            assign(left & data);
            break;
        case OR:
            assign(left | data);
            break;
        case XOR:
            assign(left ^ data);
            break;
        case NOT:
            assign(~left);
            break;
        case SIGN:
            sr = (sr & ~N) | (data & (e.width == 1 ? 0x80 : 0x8000) ? N : 0);
            break;
        case PAR:
            sr = (sr & ~Z) | ((std::popcount(unsigned(data)) % 2 == 0) ? Z : 0);
            break;
        case SHL:
        case SHR:
        {
            unsigned bits = e.width * 8;
            uint16_t value = left;
            // Avoid undefined host shifts. Shifts beyond width yield zero carry/data.
            if (data)
            {
                bool carry = data <= bits && (op == SHL ? (left >> (bits - data)) & 1 : (left >> (data - 1)) & 1);
                sr = (sr & ~C) | (carry ? C : 0);
                value = data >= bits ? 0 : (op == SHL ? left << data : left >> data);
            }
            set_register(e.reg, value, e.width);
            break;
        }
        case LOAD:
        case TRAN:
            assign(data);
            break;
        case STORE:
            write(address, data, e.width);
            nz(data, e.width);
            break;
        case SWAP:
            std::swap(reg[e.reg], reg[e.source]);
            break;
        case LDSR:
        {
            uint8_t saved = data;
            if (!(sr & S))
                sr = (saved & ~(S | I | T)) | (sr & (S | I | T));
            else
                sr = saved;
            break;
        }
        case LDSP:
            sp = data;
            break;
        case LDBP:
            bp = data;
            break;
        case STSR:
            write(address, sr, 1);
            break;
        case STSP:
            write(address, sp);
            break;
        case STBP:
            write(address, bp);
            break;
        case TRSR:
            set_register(e.reg, sr, 1);
            break;
        case TRSP:
            assign(sp);
            break;
        case TRBP:
            assign(bp);
            break;
        case SILI:
        case SIHI:
        case SELI:
        case SEHI:
        case SBP:
        case STP:
            mtu[op - SILI] = data;
            break;
        case TILI:
        case TIHI:
        case TELI:
        case TEHI:
        case TBP:
        case TTP:
            assign(mtu[op - TILI]);
            break;
        case LCPID:
            cpuid = data;
            break;
        case TCPID:
            assign(cpuid);
            break;
        case TIME:
            timer = data;
            timer_running = false;
            sr = (sr & ~Z) | (timer == 0 ? Z : 0);
            break;
        case TSTART:
            timer_running = true;
            timer_tick = std::chrono::steady_clock::now();
            break;
        case TSTOP:
            timer_running = false;
            break;
        case TRT:
            assign(timer);
            break;
        default:
            throw Fault{0};
        }
    }
    catch (const Fault &fault)
    {
        pc = instruction_pc;
        try
        {
            interrupt(4, fault.code);
        }
        catch (const std::exception &e)
        {
            failure = e.what();
            killed = true;
            running = false;
        }
        catch (...)
        {
            failure = "Fault while entering NMI handler";
            killed = true;
            running = false;
        }
    }
    catch (const std::exception &e)
    {
        failure = e.what();
        killed = true;
        running = false;
    }
}

void CPU::request_soft_reset()
{
    std::lock_guard lock(mutex);
    reset_requested = true;
}
void CPU::request_shutdown()
{
    std::lock_guard lock(mutex);
    shutdown_requested = true;
}

CPUDebugState CPU::debug_state() const
{
    std::lock_guard lock(mutex);
    return {running, killed, instruction_count, stop_serial, breakpoint_hit, temporary_hit};
}
void CPU::set_breakpoint(uint16_t address, bool enabled)
{
    std::lock_guard lock(mutex);
    breakpoints[address] = enabled;
}
bool CPU::remove_breakpoint(uint16_t address)
{
    std::lock_guard lock(mutex);
    return breakpoints.erase(address) != 0;
}
void CPU::clear_breakpoints()
{
    std::lock_guard lock(mutex);
    breakpoints.clear();
}
std::map<uint16_t, bool> CPU::list_breakpoints() const
{
    std::lock_guard lock(mutex);
    return breakpoints;
}
void CPU::run_until(uint16_t address, std::optional<uint16_t> stack)
{
    std::lock_guard lock(mutex);
    temporary_pc = address;
    temporary_sp = stack;
    start();
}
void CPU::debug_set(const std::string &name, uint16_t value)
{
    std::lock_guard lock(mutex);
    if (name.size() == 2 && (name[0] == 'r' || name[0] == 'l') && name[1] >= '0' && name[1] <= '7')
    {
        if (name[0] == 'l' && value > 0xff)
            throw std::invalid_argument("Low registers accept one byte");
        set_register(name[1] - '0', value, name[0] == 'l' ? 1 : 2);
    }
    else if (name == "pc")
        pc = value;
    else if (name == "sp")
        sp = value;
    else if (name == "bp")
        bp = value;
    else if (name == "sr" && value <= 0xff)
        sr = value;
    else
        throw std::invalid_argument("Expected r0..r7, l0..l7, pc, sp, bp or byte sr");
    running = false;
    if (breakpoint_hit && *breakpoint_hit != pc)
        breakpoint_hit.reset();
    resume_breakpoint.reset();
    temporary_pc.reset();
    temporary_sp.reset();
    temporary_hit = false;
}

std::vector<uint8_t> CPU::memory(uint16_t address, size_t size) const
{
    std::lock_guard lock(mutex);
    if (size > MAX_MEM - unsigned(address))
        throw std::out_of_range("Memory range exceeds IMEM");
    return {imem.begin() + address, imem.begin() + address + size};
}
