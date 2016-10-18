///* 
// * File:   BlockingMPMCQueue.hpp
// * Author: Barath Kannan
// * This is an unbounded multi-producer multi-consumer queue.
// * It can also be used as any combination of single-producer and single-consumer
// * queues for additional performance gains in those contexts. The queue is implemented
// * as a linked list, where nodes are stored in a freelist after being dequeued. 
// * Enqueue operations will either acquire items from the freelist or allocate a
// * new node if none are available.
// * Created on 17 October 2016, 6:33 PM
// */
//
//#ifndef CONQ_BLOCKINGMPMCQUEUE_HPP
//#define CONQ_BLOCKINGMPMCQUEUE_HPP
//
//#include "conq/MPMCQueue.hpp"
//
//#include <atomic>
//#include <thread>
//
//namespace conq{
//template<typename T>
//class BlockingMPMCQueue : private MPMCQueue<T>{
//public:
//    
//    BlockingMPMCQueue(){}
//    
//    void spEnqueue(const T& input){
//        MPMCQueue<T>::spEnqueue(input);
//        if (waitingReaders.load(std::memory_order_acquire)) c.notify_one();
//    }
//    
//    void mpEnqueue(const T& input){
//        MPMCQueue<T>::mpEnqueue(input);
//        if (waitingReaders.load(std::memory_order_acquire)) c.notify_one();
//    }
//    
//    void mcDequeue(T& output){
//        if (MPMCQueue<T>::mcDequeue(output)) return;
//        waitingReaders.fetch_add(1, std::memory_order_release);
//        static thread_local std::mutex m;
//        std::unique_lock<std::mutex> lock(m);
//        while (!MPMCQueue<T>::mcDequeueLight(output)){
//            c.wait(lock);
//        }
//    }
//    
//private:    
//    //std::mutex m;
//    std::condition_variable c;
//    std::atomic<size_t> waitingReaders{0};
//};
//}
//#endif
