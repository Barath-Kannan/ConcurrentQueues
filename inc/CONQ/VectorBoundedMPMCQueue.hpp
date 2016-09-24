/* 
 * File:   VectorBoundedMPMCQueue.hpp
 * Author: Barath Kannan
 * This is a bounded multi-producer multi-consumer queue. It can also be used as
 * any combination of single-producer and single-consumer queues for additional 
 * performance gains in those contexts. This class behaves identically to the 
 * bounded MPMCQueue except that it uses a vector as it's backing store instead
 * of an array so the size can be specified at run time instead of compile time.
 * Created on 24 September 2016, 8:11 PM
 */

#ifndef CONQ_VECTORBOUNDEDMPMCQUEUE_H
#define CONQ_VECTORBOUNDEDMPMCQUEUE_H

#include <atomic>
#include <type_traits>
#include <stdexcept>
#include <vector>

namespace CONQ{
template<typename T>
class VectorBoundedMPMCQueue
{
public:

    VectorBoundedMPMCQueue(size_t N) : _buffer(N){
        if ((N == 0) || ((N & (~N + 1)) != N)){
            throw std::length_error("size of VectorBoundedMPMCQueue must be power of 2");
        }
        for (size_t i=0; i<N; ++i){
            _buffer[i].seq.store(i, std::memory_order_relaxed);
        }
    }

    bool spEnqueue(const T& data){
        size_t head_seq = _head_seq.load(std::memory_order_relaxed);
        node_t* node = &_buffer[head_seq & (_buffer.size()-1)];
        size_t node_seq = node->seq.load(std::memory_order_acquire);
        intptr_t dif = (intptr_t) node_seq - (intptr_t) head_seq;
        if (dif == 0 && _head_seq.compare_exchange_strong(head_seq, head_seq + 1, std::memory_order_relaxed)){
            node->data = data;
            node->seq.store(head_seq + 1, std::memory_order_release);
            return true;
        }
        return false;
    }
    
    bool mpEnqueue(const T& data){
        size_t head_seq = _head_seq.load(std::memory_order_relaxed);
        while(true){
            node_t* node = &_buffer[head_seq & (_buffer.size()-1)];
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
        node_t* node = &_buffer[tail_seq & (_buffer.size()-1)];
        size_t node_seq = node->seq.load(std::memory_order_acquire);
        intptr_t dif = (intptr_t) node_seq - (intptr_t)(tail_seq + 1);
        if (dif == 0 && _tail_seq.compare_exchange_strong(tail_seq, tail_seq + 1, std::memory_order_relaxed)){
            data = std::move(node->data);
            node->seq.store(tail_seq + _buffer.size(), std::memory_order_release);
            return true;
        }
        return false;
    }
    
    bool mcDequeue(T& data){
        size_t tail_seq = _tail_seq.load(std::memory_order_relaxed);
        while(true){
            node_t* node = &_buffer[tail_seq & (_buffer.size()-1)];
            size_t node_seq = node->seq.load(std::memory_order_acquire);
            intptr_t dif = (intptr_t) node_seq - (intptr_t)(tail_seq + 1);
            if (dif == 0){
                if (_tail_seq.compare_exchange_weak(tail_seq, tail_seq + 1, std::memory_order_relaxed)){
                    data = std::move(node->data);
                    node->seq.store(tail_seq + _buffer.size(), std::memory_order_release);
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
    std::vector<node_t> _buffer;
    
    char pad0[64];
    std::atomic<size_t> _head_seq{0};
    char pad1[64];
    std::atomic<size_t> _tail_seq{0};
    
    VectorBoundedMPMCQueue(const VectorBoundedMPMCQueue&) {}
    void operator=(const VectorBoundedMPMCQueue&) {}
};

}

#endif /* CONQ_VECTORBOUNDEDMPMCQUEUE_H */

