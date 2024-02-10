#pragma once

#include <cpu.hh>
#include <emem.hh>
#include <console.hh>
#include <video.hh>

void debug_console(ANC216::CPU&, ANC216::EmemMapper&, ANC216::Video::Window&);
void print_debug_help();
void show_cpu_info(ANC216::CPUInfo);