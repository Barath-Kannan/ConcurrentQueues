/* 
 * File:   array_queue.hpp
 * Author: Barath Kannan
 * This is a bounded multi-producer multi-consumer queue. It can also be used as
 * any combination of single-producer and single-consumer queues for additional 
 * performance gains in those contexts. The queue is an implementation of Dmitry
 * Vyukov's bounded queue and should be used whenever an unbounded queue is not
 * necessary, as it will generally have much better cache locality. The size of
 * the queue must be a power of 2.
 * Created on 3 September 2016, 2:49 PM
 */

#ifndef BK_CONQ_ARRAYQUEUE_HPP
#define BK_CONQ_ARRAYQUEUE_HPP

#include <atomic>
#include <type_traits>
#include <bk_conq/bounded_queue.hpp>

namespace bk_conq{
template<typename T, size_t N>
class array_queue : public bounded_queue<T>{
public:

    array_queue(){
        static_assert((N != 0) && ((N & (~N + 1)) == N), "size of array_queue must be power of 2");
        for (size_t i=0; i<N; ++i) _buffer[i].seq.store(i, std::memory_order_relaxed);
    }
    
    array_queue(const array_queue&) = delete;
    void operator=(const array_queue&) = delete;
    
    bool sp_enqueue(T&& input){
        return sp_enqueue_forward(std::move(input));
    }
    
    bool sp_enqueue(const T& input){
        return sp_enqueue_forward(input);
    }
    
    bool mp_enqueue(T&& input){
        return mp_enqueue_forward(std::move(input));
    }
    
    bool mp_enqueue(const T& input){
        return mp_enqueue_forward(input);
    }
    
    bool sc_dequeue(T& data){
        size_t tail_seq = _tail_seq.load(std::memory_order_relaxed);
        node_t* node = &_buffer[tail_seq & (N-1)];
        size_t node_seq = node->seq.load(std::memory_order_acquire);
        intptr_t dif = (intptr_t) node_seq - (intptr_t)(tail_seq + 1);
        if (dif == 0 && _tail_seq.compare_exchange_strong(tail_seq, tail_seq + 1, std::memory_order_relaxed)){
            data = std::move(node->data);
            node->seq.store(tail_seq + N, std::memory_order_release);
            return true;
        }
        return false;
    }
    
    bool mc_dequeue(T& data){
        while(true){
            size_t tail_seq = _tail_seq.load(std::memory_order_relaxed);
            node_t* node = &_buffer[tail_seq & (N-1)];
            size_t node_seq = node->seq.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t) node_seq - (intptr_t)(tail_seq + 1);
            if (dif == 0){
                if (_tail_seq.compare_exchange_weak(tail_seq, tail_seq + 1, std::memory_order_relaxed)){
                    data = std::move(node->data);
                    node->seq.store(tail_seq + N, std::memory_order_release);
                    return true;
                }
            }
            else if (dif < 0){
                return false;
            }
        }
    }

private:
    struct node_t{
        T                     data;
        std::atomic<size_t>   seq;
    };
    
    template <typename U>
    bool sp_enqueue_forward(U&& input){
        size_t head_seq = _head_seq.load(std::memory_order_relaxed);
        node_t* node = &_buffer[head_seq & (N-1)];
        size_t node_seq = node->seq.load(std::memory_order_acquire);
        intptr_t dif = (intptr_t) node_seq - (intptr_t) head_seq;
        if (dif == 0 && _head_seq.compare_exchange_strong(head_seq, head_seq + 1, std::memory_order_relaxed)){
            node->data = std::forward<U>(input);
            node->seq.store(head_seq + 1, std::memory_order_release);
        }
        return false;
    }
    
    template <typename U>
    bool mp_enqueue_forward(U&& input){
        while(true){
            size_t head_seq = _head_seq.load(std::memory_order_relaxed);
            node_t* node = &_buffer[head_seq & (N-1)];
            size_t node_seq = node->seq.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t) node_seq - (intptr_t) head_seq;
            if (dif == 0){
                if (_head_seq.compare_exchange_weak(head_seq, head_seq + 1, std::memory_order_relaxed)){
                    node->data = std::forward<U>(input);
                    node->seq.store(head_seq + 1, std::memory_order_release);
                    return true;
                }
            }
            else if (dif < 0){
                return false;
            }
        }
    }
    
    std::array<node_t, N>   _buffer;
    char                    _pad0[64];
    std::atomic<size_t>     _head_seq{0};
    char                    _pad1[64];
    std::atomic<size_t>     _tail_seq{0};
};
} //namespace bk_conq

#endif /* BK_CONQ_ARRAYQUEUE_HPP */

