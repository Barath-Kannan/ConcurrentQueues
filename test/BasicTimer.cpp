#include "BasicTimer.h"
#include <chrono>

BasicTimer::BasicTimer()
    : running(false) {}

double BasicTimer::getElapsedSeconds() const{
    return std::chrono::duration<double>(getElapsedDuration()).count();
}

double BasicTimer::getElapsedMilliseconds() const{
    return std::chrono::duration<double, std::milli>(getElapsedDuration()).count();
}

double BasicTimer::getElapsedMicroseconds() const{
    return std::chrono::duration<double, std::micro>(getElapsedDuration()).count();
}

double BasicTimer::getElapsedNanoseconds() const{
    return std::chrono::duration<double, std::nano>(getElapsedDuration()).count();
}

std::chrono::duration<double> BasicTimer::getElapsedDuration() const{
    if (!running){
        std::chrono::duration<double> elapsed = end-begin;
        return elapsed;
    }
    auto n = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = n-begin;
    return elapsed;
}

std::ostream& operator<<(std::ostream& os, const BasicTimer& bt) {
    double val;
    if ((val = bt.getElapsedSeconds()) > 1.0){
        os << val << " seconds";
    }
    else if ((val = bt.getElapsedMilliseconds()) > 1.0){
        os << val << " milliseconds";
    }
    else if ((val = bt.getElapsedMicroseconds()) > 1.0){
        os << val << " microseconds";
    }
    else{
        val = bt.getElapsedNanoseconds();
        os << val << " nanoseconds";
    }
    return os;
}

bool BasicTimer::isRunning() const{
    return running;
}

bool BasicTimer::start() {
    if (running) return false;
    running = true;
    begin = std::chrono::high_resolution_clock::now();
    return true;
}

bool BasicTimer::stop() {
    if (!running) return false;
    end = std::chrono::high_resolution_clock::now();
    running = false;
    return true;
}