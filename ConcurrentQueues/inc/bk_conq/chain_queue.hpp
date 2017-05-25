/*
* File:   chain_queue.hpp
* Author: Barath Kannan
* This is an unbounded multi-producer multi-consumer queue.
* It can also be used as any combination of single-producer and single-consumer
* queues for additional performance gains in those contexts. The queue is implemented
* as a linked list, where nodes are stored in a freelist after being dequeued.
* Enqueue operations will either acquire items from the freelist or allocate a
* new node if none are available.
* Created on 27 August 2016, 11:30 PM
*/

#ifndef BK_CONQ_CHAINQUEUE_HPP
#define BK_CONQ_CHAINQUEUE_HPP

#include <atomic>
#include <thread>
#include <vector>
#include <array>
#include <memory>
#include <iostream>
#include <bk_conq/unbounded_queue.hpp>

namespace bk_conq {

template<typename T>
class chain_queue : public unbounded_queue<T, chain_queue<T>> {
    friend unbounded_queue<T, chain_queue<T>>;
    static const size_t BLOCK_SIZE = 1024;
public:
    chain_queue() {
        auto hnode = new list_node_t;
        auto flnode = new list_node_t;
        auto ipnode = new list_node_t;

        _head.store(hnode);
        _tail.store(_head.load(std::memory_order_relaxed), std::memory_order_relaxed);
        _free_list_head.store(flnode);
        _free_list_tail.store(_free_list_head.load(std::memory_order_relaxed), std::memory_order_relaxed);
        _in_progress_head.store(ipnode);
        _in_progress_tail.store(_in_progress_head.load(std::memory_order_relaxed), std::memory_order_relaxed);
    }

    virtual ~chain_queue() {
        list_node_t* next;
        list_node_t* tail = _tail.load(std::memory_order_relaxed);

        for (next = tail->next.load(std::memory_order_relaxed); next != nullptr; ) {
            tail = next;
            next = next->next.load(std::memory_order_relaxed);
            delete tail;
        }
        delete _tail.load();

        tail = _free_list_tail.load(std::memory_order_relaxed);
        for (next = tail->next.load(std::memory_order_relaxed); next != nullptr; ) {
            tail = next;
            next = next->next.load(std::memory_order_relaxed);
            delete tail;
        }
        delete _free_list_tail.load();

        tail = _in_progress_tail.load(std::memory_order_relaxed);
        for (next = tail->next.load(std::memory_order_relaxed); next != nullptr; ) {
            tail = next;
            next = next->next.load(std::memory_order_relaxed);
            delete tail;
        }
        delete _in_progress_tail.load();

    }

    chain_queue(const chain_queue&) = delete;
    void operator=(const chain_queue&) = delete;

protected:
    template <typename R>
    void sp_enqueue_impl(R&& input) {
        list_node_t *node = acquire_or_allocate(std::forward<R>(input));
        if (!node) return;
        _head.load(std::memory_order_relaxed)->next.store(node, std::memory_order_release);
        _head.store(node, std::memory_order_relaxed);
    }

    template <typename R>
    void mp_enqueue_impl(R&& input) {
        list_node_t *node = acquire_or_allocate(std::forward<R>(input));
        if (!node) return;
        list_node_t* prev_head = _head.exchange(node, std::memory_order_acq_rel);
        prev_head->next.store(node, std::memory_order_release);
    }

    bool sc_dequeue_impl(T& output) {
        list_node_t* tail = _tail.load(std::memory_order_relaxed);
        return dequeue_common(tail, output);
    }

    //spin on dequeue contention
    bool mc_dequeue_impl(T& output) {
        list_node_t *tail;
        for (tail = _tail.exchange(nullptr, std::memory_order_acq_rel); !tail; tail = _tail.exchange(nullptr, std::memory_order_acq_rel)) {
            std::this_thread::yield();
        }
        return dequeue_common(tail, output);
    }

    //return false on dequeue contention
    bool mc_dequeue_uncontended_impl(T& output) {
        list_node_t *tail = _tail.exchange(nullptr, std::memory_order_acq_rel);
        if (!tail) return false;
        return dequeue_common(tail, output);
    }

private:

    struct list_node_t {
        std::array<T, BLOCK_SIZE> data;
        std::atomic<list_node_t*> next{ nullptr };

        size_t indx{ 0 };
        list_node_t() {}
        template <typename R>
        bool put_get_full(R&& item) {
            data[indx] = std::forward<R>(item);
            return (++indx == BLOCK_SIZE);
        }
    };

    void list_enqueue(list_node_t* item, std::atomic<list_node_t*>& head) {
        item->next.store(nullptr, std::memory_order_relaxed);
        list_node_t * prev_head = head.exchange(item, std::memory_order_acq_rel);
        prev_head->next.store(item, std::memory_order_release);
    }

    list_node_t* list_dequeue(std::atomic<list_node_t*>& tail) {
        list_node_t *item;
        for (item = tail.exchange(nullptr, std::memory_order_acq_rel); !item; item = tail.exchange(nullptr, std::memory_order_acq_rel)) {
            std::this_thread::yield();
        }
        list_node_t* next = item->next.load(std::memory_order_acquire);
        if (!next) {
            tail.store(item, std::memory_order_release);
            return nullptr;
        }
        tail.store(next, std::memory_order_release);
        return item;
    }

    bool dequeue_common(list_node_t* tail, T& output) {
        //get item directly from tail
        if (tail->indx != 0) { //success
            output = std::move(tail->data[--tail->indx]);
            if (tail->indx == 0) { //if we are now empty
                list_node_t *next = tail->next.load(std::memory_order_acquire);
                if (next) { //we only want to progress if tail->next is valid
                    _tail.store(next, std::memory_order_release);
                    freelist_enqueue(tail);
                    return true;
                }
            }
            //either more elements or next node is nullptr so can't progress
            _tail.store(tail, std::memory_order_release);
            return true;
        }

        //nothing in tail, go next
        list_node_t *next = tail->next.load(std::memory_order_acquire);
        if (!next) {
            //restore the tail so that other dequeue operations can progress
            _tail.store(tail, std::memory_order_release);
            return try_get_from_inprogress(output);
        }
        freelist_enqueue(tail);
        return dequeue_common(next, output);
    }


    void freelist_enqueue(list_node_t *item) {
        list_enqueue(item, _free_list_head);
    }

    list_node_t* freelist_try_dequeue() {
        return list_dequeue(_free_list_tail);
    }

    void inprogress_enqueue(list_node_t *item) {
        list_enqueue(item, _in_progress_head);
    }

    list_node_t* inprogress_try_dequeue() {
        return list_dequeue(_in_progress_tail);
    }

    bool try_get_from_inprogress_tail(T& output) {
        list_node_t* item;
        for (item = _in_progress_tail.exchange(nullptr, std::memory_order_acq_rel); !item; item = _in_progress_tail.exchange(nullptr, std::memory_order_acq_rel)) {
            std::this_thread::yield();
        }
        if (item->indx != 0) { //success
            output = std::move(item->data[--item->indx]);
            if (item->indx == 0) { //if we are now empty
                list_node_t *next = item->next.load(std::memory_order_acquire);
                if (next) { //we only want to progress if tail->next is valid
                    _in_progress_tail.store(next, std::memory_order_release);
                    freelist_enqueue(item);
                    return true;
                }
            }
            //either more elements or next node is nullptr so can't progress
            _in_progress_tail.store(item, std::memory_order_release);
            return true;
        }
        _in_progress_tail.store(item, std::memory_order_release);

        return false;
    }

    bool try_get_from_inprogress(T& output) {
        list_node_t* item;
        item = inprogress_try_dequeue();
        if (item == nullptr) {
            return try_get_from_inprogress_tail(output);
        }

        if (item->indx != 0) { //success
            output = std::move(item->data[--item->indx]);
            if (item->indx == 0) { //if we are now empty
                freelist_enqueue(item);
            }
            else {
                item->next.store(nullptr, std::memory_order_relaxed);
                list_node_t* prev_head = _head.exchange(item, std::memory_order_acq_rel);
                prev_head->next.store(item, std::memory_order_release);
            }
            return true;
        }
        freelist_enqueue(item);
        return try_get_from_inprogress(output);
    }


    list_node_t* allocate() {
        return new list_node_t;
    }

    template<typename R>
    list_node_t *acquire_or_allocate(R&& input) {
        list_node_t* node = inprogress_try_dequeue(); //try get space from the in progress enqueue operations
        if (!node) node = freelist_try_dequeue();//attempt to recycle previously used storage 
        if (!node) node = allocate(); //allocate new storage
        if (node->put_get_full(std::forward<R>(input))) {
            node->next.store(nullptr, std::memory_order_relaxed);
            return node;
        }
        inprogress_enqueue(node);
        return nullptr;
    }

    std::atomic<list_node_t*> _in_progress_head;
    std::atomic<list_node_t*> _head;
    std::atomic<list_node_t*> _free_list_tail;
    std::array<char, 64> _padding1;
    std::atomic<list_node_t*> _tail;
    std::atomic<list_node_t*> _free_list_head;
    std::array<char, 64> _padding2;
    std::atomic<list_node_t*> _in_progress_tail;
};
}//namespace bk_conq

#endif /* BK_CONQ_CHAINQUEUE_HPP */
