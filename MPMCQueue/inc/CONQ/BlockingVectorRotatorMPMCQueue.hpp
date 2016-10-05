/* 
 * File:   BlockingVectorRotatorMPMCQueue.hpp
 * Author: Barath Kannan
 *
 * Created on 4 October 2016, 11:35 AM
 */

#ifndef BLOCKINGVECTORROTATORMPMCQUEUE_HPP
#define BLOCKINGVECTORROTATORMPMCQUEUE_HPP

#include <mutex>
#include <condition_variable>
#include "CONQ/VectorRotatorMPMCQueue.hpp"

namespace CONQ{

template <typename T>
class BlockingVectorRotatorMPMCQueue : private VectorRotatorMPMCQueue<T>{
public:
    BlockingVectorRotatorMPMCQueue(size_t subqueues)
        : VectorRotatorMPMCQueue<T>(subqueues){}
    
    void enqueue(const T& input){
        VectorRotatorMPMCQueue<T>::mpEnqueue(input);
        if (waitingReaders.load(std::memory_order_acquire)) c.notify_one();
    }
    
    bool try_dequeue(T& output){
        return VectorRotatorMPMCQueue<T>::mcDequeue(output);
    }
    
    void dequeue(T& output){
        waitingReaders.fetch_add(1, std::memory_order_release);
        std::unique_lock<std::mutex> lock(m);
        while (!try_dequeue(output)){
            c.wait(lock);
        }
        waitingReaders.fetch_sub(1, std::memory_order_release);
    }
    
private:
    std::mutex m;
    std::condition_variable c;
    std::atomic<size_t> waitingReaders{0};
};

}

#endif /* BLOCKINGVECTORROTATORMPMCQUEUE_HPP */

