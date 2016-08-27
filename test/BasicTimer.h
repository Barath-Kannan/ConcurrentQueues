/* 
 * File:   BasicTimer.h
 * Author: Barath Kannan
 *
 * Created on April 21, 2016, 12:09 PM
 */

#ifndef BSIGNALS_BASICTIMER_H
#define BSIGNALS_BASICTIMER_H

#include <chrono>
#include <iostream>

class BasicTimer{
public:
    BasicTimer();
    bool start();
    bool stop();
    
    bool isRunning() const;
    double getElapsedSeconds() const;
    double getElapsedMilliseconds() const;
    double getElapsedMicroseconds() const;
    double getElapsedNanoseconds() const;
    
    std::chrono::duration<double> getElapsedDuration() const;
    friend std::ostream& operator<< (std::ostream& os, const BasicTimer& bt) ;
    
protected:
    std::chrono::time_point<std::chrono::high_resolution_clock> begin, end;
    bool running;
};

#endif /* BSIGNALS_BASICTIMER_H */
