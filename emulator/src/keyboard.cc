#include <keyboard.hh>

using namespace ANC216;
void Keyboard::input(uint16_t value)
{
    std::lock_guard lock(mutex);
    if (!value)
        return;
    if (events.size() == 64)
    {
        if (dropped != 0xffff)
            ++dropped;
        return;
    }
    events.push_back(value);
}
uint16_t Keyboard::pending()
{
    std::lock_guard lock(mutex);
    return events.empty() ? 0 : events.front();
}
void Keyboard::consume()
{
    std::lock_guard lock(mutex);
    if (!events.empty())
        events.pop_front();
}
void Keyboard::cpu_write(uint16_t value, bool)
{
    std::lock_guard lock(mutex);
    if (value == 0)
    {
        events.clear();
        dropped = 0;
    }
}
uint16_t Keyboard::cpu_read(uint16_t request, bool)
{
    std::lock_guard lock(mutex);
    if (request == 1)
        return events.size();
    if (request == 2)
        return dropped;
    if (request != 0 || events.empty())
        return 0;
    auto value = events.front();
    events.pop_front();
    return value;
}
