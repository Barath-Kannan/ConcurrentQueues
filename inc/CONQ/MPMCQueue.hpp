/* 
 * File:   MPMCQueue.hpp
 * Author: Barath Kannan
 * Created on 27 August 2016, 11:30 PM
 */

#ifndef CONQ_MPMCQUEUE_HPP
#define CONQ_MPMCQUEUE_HPP

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace CONQ{

template<typename T>
class MPMCQueue{
public:
    MPMCQueue(){
        _freeListTail.store(_freeListHead.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }
    
    ~MPMCQueue(){
        T output;
        while(this->scDequeue(output));
        listNode* front = _head.load(std::memory_order_relaxed);
        delete front;
        for (listNode *front = freeListTryDequeue(); front != nullptr; front = freeListTryDequeue()) delete front;
        front = _freeListHead.load(std::memory_order_relaxed);
        delete front;
    }
    
    void spEnqueue(const T& input){
        listNode *node  = acquireOrAllocate(input);
        _head.load(std::memory_order_relaxed)->next.store(node, std::memory_order_release);
        _head.store(node, std::memory_order_relaxed);
    }
    
    void mpEnqueue(const T& input){
        listNode *node  = acquireOrAllocate(input);
        listNode* prev_head = _head.exchange(node, std::memory_order_acq_rel);
        prev_head->next.store(node, std::memory_order_release);
    }
    
    bool scDequeue(T& output){
        listNode* tail = _tail.load(std::memory_order_relaxed);
        listNode* next = tail->next.load(std::memory_order_acquire);
        if (next == nullptr){
            return false;
        }
        output = std::move(next->data);
        _tail.store(next, std::memory_order_release);
        freeListEnqueue(tail);
        return true;
    }
    
    bool mcDequeue(T& output){
        listNode *tail = _tail.exchange(nullptr, std::memory_order_acq_rel);
        while (!tail){
            std::this_thread::yield();
            tail = _tail.exchange(nullptr, std::memory_order_acq_rel);
        }
        listNode *next = tail->next.load(std::memory_order_acquire);
        if (next == nullptr){
            _tail.exchange(tail, std::memory_order_acq_rel);
            return false;
        }
        output = std::move(next->data);
        _tail.store(next, std::memory_order_release);
        freeListEnqueue(tail);
        return true;
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
    
    inline listNode *acquireOrAllocate(const T& input){
        listNode *node  = freeListTryDequeue();
        if (!node){
            node = new listNode{input};
        }
        else{
            node->data = input;
            node->next.store(nullptr, std::memory_order_relaxed);
        }
        return node;
    }
    
    std::atomic<listNode*> _head{new listNode};
    std::atomic<listNode*> _freeListTail;
    char padding[64];
    std::atomic<listNode*> _tail{_head.load(std::memory_order_relaxed)};
    std::atomic<listNode*> _freeListHead{new listNode};
    MPMCQueue(const MPMCQueue&) {}
    void operator=(const MPMCQueue&) {}
};
}

#endif
