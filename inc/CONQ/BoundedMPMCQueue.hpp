/* 
 * File:   BoundedMPMCQueue.hpp
 * Author: Barath Kannan
 * This is a bounded multi-producer multi-consumer queue. It can also be used as
 * any combination of single-producer and single-consumer queues for additional 
 * performance gains in those contexts. The queue is an implementation of Dmitry
 * Vyukov's bounded queue and should be used whenever an unbounded queue is not
 * necessary, as it will generally have much better cache locality. The size of
 * the queue must be a power of 2.
 * Created on 3 September 2016, 2:49 PM
 */

#ifndef CONQ_BOUNDEDMPMCQUEUE_HPP
#define CONQ_BOUNDEDMPMCQUEUE_HPP

#include <atomic>
#include <type_traits>

namespace CONQ{

template<typename T, size_t N>
class BoundedMPMCQueue
{
public:

    BoundedMPMCQueue(){
        static_assert((N != 0) && ((N & (~N + 1)) == N), "size of BoundedMPMCQueue must be power of 2");
        for (size_t i=0; i<N; ++i){
            _buffer[i].seq.store(i, std::memory_order_relaxed);
        }
    }

    bool spEnqueue(const T& data){
        size_t head_seq = _head_seq.load(std::memory_order_relaxed);
        node_t* node = &_buffer[head_seq & (N-1)];
        size_t node_seq = node->seq.load(std::memory_order_acquire);
        intptr_t dif = (intptr_t) node_seq - (intptr_t) head_seq;
        if (dif == 0 && _head_seq.compare_exchange_weak(head_seq, head_seq + 1, std::memory_order_relaxed)){
            node->data = data;
            node->seq.store(head_seq + 1, std::memory_order_release);
            return true;
        }
        return false;
    }
    
    bool mpEnqueue(const T& data){
        size_t head_seq = _head_seq.load(std::memory_order_relaxed);
        while(true){
            node_t* node = &_buffer[head_seq & (N-1)];
            size_t node_seq = node->seq.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t) node_seq - (intptr_t) head_seq;
            if (dif == 0){
                if (_head_seq.compare_exchange_weak(head_seq, head_seq + 1, std::memory_order_relaxed)){
                    node->data = data;
                    node->seq.store(head_seq + 1, std::memory_order_release);
                    return true;
                }
            }
            else if (dif < 0){
                return false;
            }
            else{
                head_seq = _head_seq.load(std::memory_order_relaxed);
            }
        }
    }

    bool scDequeue(T& data){
        size_t tail_seq = _tail_seq.load(std::memory_order_relaxed);
        node_t* node = &_buffer[tail_seq & (N-1)];
        size_t node_seq = node->seq.load(std::memory_order_acquire);
        intptr_t dif = (intptr_t) node_seq - (intptr_t)(tail_seq + 1);
        if (dif == 0 && _tail_seq.compare_exchange_weak(tail_seq, tail_seq + 1, std::memory_order_relaxed)){
            data = node->data;
            node->seq.store(tail_seq + N, std::memory_order_release);
            return true;
        }
        return false;
    }
    
    bool mcDequeue(T& data){
        size_t tail_seq = _tail_seq.load(std::memory_order_relaxed);
        while(true){
            node_t* node = &_buffer[tail_seq & (N-1)];
            size_t node_seq = node->seq.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t) node_seq - (intptr_t)(tail_seq + 1);
            if (dif == 0){
                if (_tail_seq.compare_exchange_weak(tail_seq, tail_seq + 1, std::memory_order_relaxed)){
                    data = node->data;
                    node->seq.store(tail_seq + N, std::memory_order_release);
                    return true;
                }
            }
            else if (dif < 0){
                return false;
            }
            else{
                tail_seq = _tail_seq.load(std::memory_order_relaxed);
            }
        }
    }

private:
    struct node_t{
        T                     data;
        std::atomic<size_t>   seq;
    };

    std::array<node_t, N> _buffer;
    char pad0[64];
    std::atomic<size_t> _head_seq{0};
    char pad1[64];
    std::atomic<size_t> _tail_seq{0};
    
    BoundedMPMCQueue(const BoundedMPMCQueue&) {}
    void operator=(const BoundedMPMCQueue&) {}
};
}
#endif /* CONQ_BOUNDEDMPMCQUEUE_H */

