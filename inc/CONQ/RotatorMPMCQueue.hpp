/* 
 * File:   RotatorMPMCQueue.hpp
 * Author: Barath Kannan
 *
 * Created on 25 September 2016, 12:04 AM
 */

#ifndef CONQ_ROTATORMPMCQUEUE_HPP
#define CONQ_ROTATORMPMCQUEUE_HPP

#include "CONQ/MPMCQueue.hpp"
#include <thread>

namespace CONQ{

template <typename T, size_t SUBQUEUES>
class RotatorMPMCQueue{
public:
    RotatorMPMCQueue(){}
    
    void mpEnqueue(const T& input){
        thread_local static size_t indx{acquireEnqueueIndx()};
        q[indx].mpEnqueue(input);
    }
    
    bool mcDequeue(T& output){
        thread_local static size_t indx{acquireDequeueIndx()};
        for (size_t i=0; i<SUBQUEUES; ++i){
            if (q[(indx+i)%SUBQUEUES].mcDequeue(output)) return true;
        }
        return false;
    }
    
private:
    size_t acquireEnqueueIndx(){
        size_t tmp = enqueueIndx.load(std::memory_order_relaxed);
        while(!enqueueIndx.compare_exchange_weak(tmp, (tmp+1)%SUBQUEUES));
        return tmp;
    }
    
    size_t acquireDequeueIndx(){
        size_t tmp = dequeueIndx.load(std::memory_order_relaxed);
        while(!dequeueIndx.compare_exchange_weak(tmp, (tmp+1)%SUBQUEUES));
        return tmp;
    }
    
    std::atomic<size_t> enqueueIndx{0};
    std::atomic<size_t> dequeueIndx{0};
    std::array<MPMCQueue<T>, SUBQUEUES> q;
    
    RotatorMPMCQueue(const RotatorMPMCQueue&){};
    void operator=(const RotatorMPMCQueue&){};
};

}

#endif /* CONQ_ROTATORMPMCQUEUE_HPP */

