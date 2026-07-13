// Milliseconds since first call. Replaces an x87-asm RDTSC counter that was
// corrected against timeGetTime() to survive frequency scaling and cores drifting
// apart — steady_clock is that guarantee, from the platform, on every OS.
#include <chrono>

int xclock()
{
    using namespace std::chrono;
    static auto start = steady_clock::now();
    return (int)duration_cast<milliseconds>(steady_clock::now() - start).count();
}
