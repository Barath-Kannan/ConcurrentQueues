/* 
 * File:   list_queue.hpp
 * Author: Barath Kannan
 * This is an unbounded multi-producer multi-consumer queue.
 * It can also be used as any combination of single-producer and single-consumer
 * queues for additional performance gains in those contexts. The queue is implemented
 * as a linked list, where nodes are stored in a freelist after being dequeued. 
 * Enqueue operations will either acquire items from the freelist or allocate a
 * new node if none are available.
 * Created on 27 August 2016, 11:30 PM
 */

#ifndef BK_CONQ_LISTQUEUE_HPP
#define BK_CONQ_LISTQUEUE_HPP

#include <atomic>
#include <thread>
#include <bk_conq/unbounded_queue.hpp>

namespace bk_conq{
template<typename T>
class list_queue : public unbounded_queue<T>{
public:
    list_queue(){
        _free_list_tail.store(_free_list_head.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }
    
    ~list_queue(){
        T output;
        while(this->sc_dequeue(output));
        list_node_t* front = _head.load(std::memory_order_relaxed);
        delete front;
        for (list_node_t *front = freelist_try_dequeue(); front != nullptr; front = freelist_try_dequeue()) delete front;
        front = _free_list_head.load(std::memory_order_relaxed);
        delete front;
    }
    
    list_queue(const list_queue&) = delete;
    void operator=(const list_queue&) = delete;
     
    void sp_enqueue(T&& input){
        return sp_enqueue_forward(std::move(input));
    }
    
    void sp_enqueue(const T& input){
        return sp_enqueue_forward(input);
    }
    
    void mp_enqueue(T&& input){
        return mp_enqueue_forward(std::move(input));
    }
    
    void mp_enqueue(const T& input){
        return mp_enqueue_forward(input);
    }
    
    bool sc_dequeue(T& output){
        list_node_t* tail = _tail.load(std::memory_order_relaxed);
        list_node_t* next = tail->next.load(std::memory_order_acquire);
        if (!next) return false;
        output = std::move(next->data);
        _tail.store(next, std::memory_order_release);
        freelist_enqueue(tail);
        return true;
    }    

    //yield spin on dequeue contention
    bool mc_dequeue(T& output){
        list_node_t *tail;
        for (tail = _tail.exchange(nullptr, std::memory_order_acq_rel); !tail; tail = _tail.exchange(nullptr, std::memory_order_acq_rel)){
            std::this_thread::yield();
        }
        list_node_t *next = tail->next.load(std::memory_order_acquire);
        if (!next){
            _tail.exchange(tail, std::memory_order_acq_rel);
            return false;
        }
        output = std::move(next->data);
        _tail.store(next, std::memory_order_release);
        freelist_enqueue(tail);
        return true;
    }
    
    //return false on dequeue contention
    bool mc_dequeue_light(T& output){
        list_node_t *tail = _tail.exchange(nullptr, std::memory_order_acq_rel);
        if (!tail) return false;
        list_node_t *next = tail->next.load(std::memory_order_acquire);
        if (!next){
            _tail.exchange(tail, std::memory_order_acq_rel);
            return false;
        }
        output = std::move(next->data);
        _tail.store(next, std::memory_order_release);
        freelist_enqueue(tail);
        return true;
    }
    
private:    
    struct list_node_t{
        T data;
        std::atomic<list_node_t*> next{nullptr};
    };
    
    inline void freelist_enqueue(list_node_t *item){
        item->next.store(nullptr, std::memory_order_relaxed);
        list_node_t * free_list_prev_head = _free_list_head.exchange(item, std::memory_order_acq_rel);
        free_list_prev_head->next.store(item, std::memory_order_release);
    }
    
    inline list_node_t* freelist_try_dequeue(){
        list_node_t* node = _free_list_tail.load(std::memory_order_relaxed);
        for (list_node_t *next = node->next.load(std::memory_order_acquire); next != nullptr; next = node->next.load(std::memory_order_acquire)){
            if (_free_list_tail.compare_exchange_strong(node, next)) return node;
        }
        return nullptr;
    }
    
    template<typename U>
    inline list_node_t *acquire_or_allocate(U&& input){
        list_node_t *node = freelist_try_dequeue();
        if (!node){
            node = new list_node_t;
            node->data = std::forward<U>(input);
        }
        else{
            node->data = std::forward<U>(input);
            node->next.store(nullptr, std::memory_order_relaxed);
        }
        return node;
    }
      
    template <typename U>
    void sp_enqueue_forward(U&& input){
        list_node_t *node = acquire_or_allocate(std::forward<U>(input));
        _head.load(std::memory_order_relaxed)->next.store(node, std::memory_order_release);
        _head.store(node, std::memory_order_relaxed);
    }
    
    template <typename U>
    void mp_enqueue_forward(U&& input){
        list_node_t *node = acquire_or_allocate(std::forward<U>(input));
        list_node_t* prev_head = _head.exchange(node, std::memory_order_acq_rel);
        prev_head->next.store(node, std::memory_order_release);
    }
    
    std::atomic<list_node_t*>   _head{new list_node_t};
    std::atomic<list_node_t*>   _free_list_tail;
    char                        _padding[64];
    std::atomic<list_node_t*>   _tail{_head.load(std::memory_order_relaxed)};
    std::atomic<list_node_t*>   _free_list_head{new list_node_t};
};
}//namespace bk_conq

#endif /* BK_CONQ_LISTQUEUE_HPP */
