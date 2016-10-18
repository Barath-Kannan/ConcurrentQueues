///* 
// * File:   BlockingConcurrentQueue.hpp
// * Author: Barath Kannan
// * Abstract specification for a blocking queue.
// * Created on 17 October 2016, 1:15 PM
// */
//
//#ifndef CONQ_BLOCKINGCONCURRENTQUEUE_HPP
//#define CONQ_BLOCKINGCONCURRENTQUEUE_HPP
//
//#include "conq/ConcurrentQueue.hpp"
//
//namespace conq{
//    
//template<typename T, ConcurrentQueue C>
//class BlockingConcurrentQueue<T> : private C<T>{
//public:
//    virtual bool try_sp_enqueue(const T& input) = 0;
//    virtual bool try_sp_enqueue(T&& input) = 0;
//    virtual bool try_mp_enqueue(const T& input) = 0;
//    virtual bool try_mp_enqueue(T&& input) = 0;
//    
//    virtual void sp_enqueue(const T& input) = 0;
//    virtual void sp_enqueue(T&& input) = 0;
//    virtual void mp_enqueue(const T& input) = 0;
//    virtual void mp_enqueue(T&& input) = 0;
//    
//    virtual bool try_sc_dequeue(T& output) = 0;
//    virtual bool try_mc_dequeue(T& output) = 0;
//    virtual void sc_dequeue(T& output) = 0;
//    virtual void mc_dequeue(T& output) = 0;
//};
//}
//
//#endif /* CONQ_BLOCKINGCONCURRENTQUEUE_HPP */
