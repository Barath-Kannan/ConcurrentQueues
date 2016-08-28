/* 
 * File:   MPMCQueue.hpp
 * Author: Barath Kannan
 * Created on 27 August 2016, 11:30 PM
 */

#ifndef CONQ_MPMCQUEUE_HPP
#define CONQ_MPMCQUEUE_HPP

#include <atomic>
#include <thread>

namespace CONQ{

template<typename T>
class MPMCQueue{
public:
    MPMCQueue(){
        _spinner.clear();
    }
    
    ~MPMCQueue(){
        T output;
        while(this->dequeue(output));
        listNode* front = _head.load(std::memory_order_relaxed);
        delete front;
        for (listNode *front = freeListTryDequeue(); front != nullptr; front = freeListTryDequeue()) delete front;
        front = _freeListHead.load(std::memory_order_relaxed);
        delete front;
    }
    
    void enqueue(const T& input){
        listNode *node  = freeListTryDequeue();
        if (!node){
            node = new listNode{input};
        }
        else{
            node->data = input;
            node->next.store(nullptr, std::memory_order_relaxed);
        }
        listNode* prev_head = _head.exchange(node, std::memory_order_acq_rel);
        prev_head->next.store(node, std::memory_order_release);
    }

    bool dequeue(T& output){
        while (_spinner.test_and_set(std::memory_order_acquire)){
            std::this_thread::yield();
        }
        listNode* tail = _tail.load(std::memory_order_relaxed);
        listNode* next = tail->next.load(std::memory_order_acquire);
        if (next == nullptr){
            _spinner.clear(std::memory_order_release);
            return false;
        }
        output = next->data;
        _tail.store(next, std::memory_order_release);
        freeListEnqueue(tail);
        _spinner.clear(std::memory_order_release);
        return true;
    }
    
    bool empty(){
        return !(_tail.load(std::memory_order_relaxed)->next.load(std::memory_order_acquire));
    }
    
private:    
    struct listNode{
        T data;
        std::atomic<listNode*> next{nullptr};
    };
    
    inline void freeListEnqueue(listNode *item){
        item->next.store(nullptr, std::memory_order_relaxed);
        listNode * free_list_prev_head = _freeListHead.exchange(item, std::memory_order_acq_rel);
        free_list_prev_head->next.store(item, std::memory_order_release);
    }
    
    inline listNode* freeListTryDequeue(){
        listNode* node = _freeListTail.load(std::memory_order_relaxed);
        for (listNode *next = node->next.load(std::memory_order_acquire); next != nullptr; next = node->next.load(std::memory_order_acquire)){
            if (_freeListTail.compare_exchange_strong(node, next)) return node;
        }
        return nullptr;
    }

    std::atomic<listNode*> _head{new listNode};
    char _padding_0[64];
    std::atomic<listNode*> _tail{_head.load(std::memory_order_relaxed)};
    std::atomic<listNode*> _freeListHead{new listNode};
    char _padding_1[64];
    std::atomic<listNode*> _freeListTail{_freeListHead.load(std::memory_order_relaxed)};
    char _padding_2[64];
    std::atomic_flag _spinner;
    MPMCQueue(const MPMCQueue&) {}
    void operator=(const MPMCQueue&) {}
};
}

#endif
