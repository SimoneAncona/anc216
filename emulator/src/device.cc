#include <device.hh>

ANC216::Device::Device(EmemMapper *emem, EmuFlags flags)
{
    this->emem = emem;
    this->flags = flags;
}

inline uint16_t ANC216::Device::get_addr() const
{
    return emem->where_am_i(this);
}

inline ANC216::DeviceID ANC216::Device::cpu_info_req()
{
    return this->id;
}