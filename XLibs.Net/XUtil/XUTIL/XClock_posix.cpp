// Cross-platform stub for xclock() — replaces the MSVC/RDTSC implementation in XClock.cpp
#include <chrono>

int xclock()
{
    using namespace std::chrono;
    static auto start = steady_clock::now();
    return (int)duration_cast<milliseconds>(steady_clock::now() - start).count();
}
