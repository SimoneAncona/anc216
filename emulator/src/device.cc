#include <device.hh>
#include <emem.hh>
ANC216::Device::Device(EmemMapper *mapper, EmuFlags options) : emem(mapper), flags(options)
{
}
uint16_t ANC216::Device::get_addr() const
{
    return emem->where_am_i(this);
}
