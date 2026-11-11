#include "../orbitris_esp32/trace.h"

#include <chrono>

void do_print(const char *msg)
{
    printf("%s", msg);
    fflush(stdout);
}

uint32_t platform_micros()
{
    using namespace std::chrono;
    static const auto start = steady_clock::now();
    return static_cast<uint32_t>(duration_cast<microseconds>(steady_clock::now() - start).count());
}
