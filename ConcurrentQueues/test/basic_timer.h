/*
 * File:   basic_timer.h
 * Author: Barath Kannan
 *
 * Created on April 21, 2016, 12:09 PM
 */

#ifndef BK_CONQ_BASICTIMER_H
#define BK_CONQ_BASICTIMER_H

#include <chrono>
#include <iostream>

class basic_timer {
public:
    basic_timer();
    bool start();
    bool stop();

    bool isRunning() const;
    double getElapsedSeconds() const;
    double getElapsedMilliseconds() const;
    double getElapsedMicroseconds() const;
    double getElapsedNanoseconds() const;

    std::chrono::duration<double> getElapsedDuration() const;
    friend std::ostream& operator<< (std::ostream& os, const basic_timer& bt);

protected:
    std::chrono::time_point<std::chrono::high_resolution_clock> begin{ std::chrono::high_resolution_clock::now() };
    std::chrono::time_point<std::chrono::high_resolution_clock> end{ begin };
    bool running;
};

#endif /* BK_CONQ_BASICTIMER_H */
