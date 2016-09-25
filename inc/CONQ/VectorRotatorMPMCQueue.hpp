/* 
 * File:   VectorRotatorMPMCQueue.hpp
 * Author: Barath Kannan
 * Vector of unbounded MPMC Queues. Enqueue operations are assigned a subqueue,
 * which is used for all enqueue operations occuring from that thread. The deque
 * operation maintains a list of subqueues on which a "hit" has occured - pertaining
 * to the subqueues from which a successful dequeue operation has occured. On a
 * successful dequeue operation, the queue that is used is pushed to the front of the
 * list. The "hit lists" allow the queue to adapt fairly well to different usage contexts,
 * including when there are more readers than writers, more writers than readers,
 * and high contention.
 * Created on 25 September 2016, 11:08PM
 */

#ifndef CONQ_VECTORROTATORMPMCQUEUE_HPP
#define CONQ_VECTORROTATORMPMCQUEUE_HPP

#include <thread>
#include <vector>
#include "CONQ/MPMCQueue.hpp"

namespace CONQ{
    
template <typename T>
class VectorRotatorMPMCQueue{
public:
    VectorRotatorMPMCQueue(size_t subqueues) : q(subqueues){}
    
    void mpEnqueue(const T& input){
        thread_local static size_t indx{enqueueIndx.fetch_add(1)%q.size()};
        q[indx].mpEnqueue(input);
    }
    
    bool mcDequeue(T& output){
        thread_local static std::vector<size_t> hitList(q.size(), q.size());
        if (hitList[0] == q.size()) std::iota(hitList.begin(), hitList.end(), 0);
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
    std::vector<MPMCQueue<T>> q;
    
    VectorRotatorMPMCQueue(const VectorRotatorMPMCQueue&){};
    void operator=(const VectorRotatorMPMCQueue&){};
};

}

#endif /* CONQ_VECTORROTATORMPMCQUEUE_HPP */

