#pragma once
#include <chrono>

// High-resolution wall-clock timer.
class Timer {
public:
    void start()  { _start = std::chrono::high_resolution_clock::now(); }
    void stop()   { _end   = std::chrono::high_resolution_clock::now(); }
    double elapsedMs() const {
        return std::chrono::duration<double, std::milli>(_end - _start).count();
    }
private:
    std::chrono::high_resolution_clock::time_point _start, _end;
};
