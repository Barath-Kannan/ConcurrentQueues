/* 
 * File:   RotatorMPMCQueue.hpp
 * Author: Barath Kannan
 * Array of unbounded MPMC Queues. Enqueue operations are assigned a subqueue,
 * which is used for all enqueue operations occuring from that thread. The deque
 * operation maintains a list of subqueues on which a "hit" has occured - pertaining
 * to the subqueues from which a successful dequeue operation has occured. On a
 * successful dequeue operation, the queue that is used is pushed to the front of the
 * list. The "hit lists" allow the queue to adapt fairly well to different usage contexts,
 * including when there are more readers than writers, more writers than readers,
 * and high contention. This queue only performs worse in single-reader single-writer
 * contexts.
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
    RotatorMPMCQueue(){
    }
    
    void mpEnqueue(const T& input){
        thread_local static size_t indx{acquireEnqueueIndx()};
        q[indx].mpEnqueue(input);
    }
    
    bool mcDequeue(T& output){
        thread_local static std::array<int, SUBQUEUES> hitList{{-1}};
        if (hitList[0] == -1){
            size_t indx{acquireDequeueIndx()};
            for (size_t i=0; i<SUBQUEUES; ++i){
                hitList[i] = ((i+indx)%SUBQUEUES);
            }
        }
        if (q[hitList[0]].mcDequeue(output)) return true;
        for (auto it = hitList.begin()+1; it != hitList.end(); ++it){
            if (q[*it].mcDequeue(output)){
                std::swap(*it, *hitList.begin());
                return true;
            }
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

