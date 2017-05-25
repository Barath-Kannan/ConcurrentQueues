#include "basic_timer.h"
#include <chrono>

basic_timer::basic_timer()
    : running(false) {}

double basic_timer::getElapsedSeconds() const {
    return std::chrono::duration<double>(getElapsedDuration()).count();
}

double basic_timer::getElapsedMilliseconds() const {
    return std::chrono::duration<double, std::milli>(getElapsedDuration()).count();
}

double basic_timer::getElapsedMicroseconds() const {
    return std::chrono::duration<double, std::micro>(getElapsedDuration()).count();
}

double basic_timer::getElapsedNanoseconds() const {
    return std::chrono::duration<double, std::nano>(getElapsedDuration()).count();
}

std::chrono::duration<double> basic_timer::getElapsedDuration() const {
    if (!running) {
        std::chrono::duration<double> elapsed = end - begin;
        return elapsed;
    }
    auto n = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = n - begin;
    return elapsed;
}

std::ostream& operator<<(std::ostream& os, const basic_timer& bt) {
    double val;
    if ((val = bt.getElapsedSeconds()) > 1.0) {
        os << val << " seconds";
    }
    else if ((val = bt.getElapsedMilliseconds()) > 1.0) {
        os << val << " milliseconds";
    }
    else if ((val = bt.getElapsedMicroseconds()) > 1.0) {
        os << val << " microseconds";
    }
    else {
        val = bt.getElapsedNanoseconds();
        os << val << " nanoseconds";
    }
    return os;
}

bool basic_timer::isRunning() const {
    return running;
}

bool basic_timer::start() {
    if (running) return false;
    running = true;
    begin = std::chrono::high_resolution_clock::now();
    return true;
}

bool basic_timer::stop() {
    if (!running) return false;
    end = std::chrono::high_resolution_clock::now();
    running = false;
    return true;
}