/* 
 * File:   MPMCCacheQueue.hpp
 * Author: Barath Kannan
 * This is a queue implementation that stores items in a bounded MPMC queue and
 * overflows into an unbounded MPMCQueue. It does not maintain order and can
 * be very unfair to the unbounded queue at high contention.
 * Created on 24 September 2016, 10:38 PM
 */

#ifndef CONQ_MPMCCACHEQUEUE_HPP
#define CONQ_MPMCCACHEQUEUE_HPP

#include "CONQ/BoundedMPMCQueue.hpp"
#include "CONQ/MPMCQueue.hpp"

namespace CONQ{

template<typename T, size_t CACHE_SIZE>
class MPMCCacheQueue{
public:
    
    void mpEnqueue(const T& input){
        if (!bq.mpEnqueue(input)){
            q.mpEnqueue(input);
        }
    }
    
    bool mcDequeue(T& output){
        if (bq.mcDequeue(output)){
            return true;
        }
        return q.mcDequeue(output);
    }
    
private:
    BoundedMPMCQueue<T, CACHE_SIZE> bq;
    MPMCQueue<T> q;
};

}

#endif /* CONQ_MPMCCACHEQUEUE_HPP */

