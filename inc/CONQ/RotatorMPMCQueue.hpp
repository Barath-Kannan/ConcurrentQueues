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
 * and high contention.
 * Created on 25 September 2016, 12:04 AM
 */

#ifndef CONQ_ROTATORMPMCQUEUE_HPP
#define CONQ_ROTATORMPMCQUEUE_HPP

#include <thread>
#include "CONQ/MPMCQueue.hpp"

namespace CONQ{
    
template <typename T, size_t SUBQUEUES>
class RotatorMPMCQueue{
public:
    RotatorMPMCQueue(){}
    
    void mpEnqueue(const T& input){
        thread_local static size_t indx{enqueueIndx.fetch_add(1)%SUBQUEUES};
        q[indx].mpEnqueue(input);
    }
    
    bool mcDequeue(T& output){
        thread_local static std::array<size_t, SUBQUEUES> hitList{{SUBQUEUES}};
        if (hitList[0] == SUBQUEUES) std::iota(hitList.begin(), hitList.end(), 0);
        for (auto it = hitList.begin(); it != hitList.end(); ++it){
            if (q[*it].mcDequeueLight(output)){
                for (auto it2 = hitList.begin(); it2 != it; ++it2) std::iter_swap(it, it2);
                return true;
            }
        }
        for (auto it = hitList.begin(); it != hitList.end(); ++it){
            if (q[*it].mcDequeue(output)){
                for (auto it2 = hitList.begin(); it2 != it; ++it2) std::iter_swap(it, it2);
                return true;
            }
        }
        return false;
    }
    
private:
    std::atomic<size_t> enqueueIndx{0};
    std::array<MPMCQueue<T>, SUBQUEUES> q;
    
    RotatorMPMCQueue(const RotatorMPMCQueue&){};
    void operator=(const RotatorMPMCQueue&){};
};

}

#endif /* CONQ_ROTATORMPMCQUEUE_HPP */

