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
#include <vector>
#include <array>
#include <memory>
#include <iostream>
#include <bk_conq/unbounded_queue.hpp>

namespace bk_conq {

template<typename T>
class list_queue : public unbounded_queue<T, list_queue<T>> {
    friend unbounded_queue<T, list_queue<T>>;
public:
    list_queue() {
        std::vector<list_node_t> vec(2);
        _head.store(&vec[0]);
        _tail.store(_head.load(std::memory_order_relaxed), std::memory_order_relaxed);
        _free_list_head.store(&vec[1]);
        _free_list_tail.store(_free_list_head.load(std::memory_order_relaxed), std::memory_order_relaxed);

        storage_node_t *store = new storage_node_t(std::move(vec));
        storage_node_t* prev_head = _storage_head.exchange(store, std::memory_order_acq_rel);
        prev_head->next.store(store, std::memory_order_release);
    }

    virtual ~list_queue() {
        storage_node_t* tail = _storage_tail.load(std::memory_order_relaxed);
        storage_node_t* next = tail->next.load(std::memory_order_relaxed);
        for (storage_node_t* next = tail->next.load(std::memory_order_relaxed); next != nullptr; ) {
            tail = next;
            next = next->next.load(std::memory_order_relaxed);
            delete tail;
        }
    }

    list_queue(const list_queue&) = delete;
    void operator=(const list_queue&) = delete;

protected:
    template <typename R>
    void sp_enqueue_impl(R&& input) {
        list_node_t *node = acquire_or_allocate(std::forward<R>(input));
        _head.load(std::memory_order_relaxed)->next.store(node, std::memory_order_release);
        _head.store(node, std::memory_order_relaxed);
    }

    template <typename R>
    void mp_enqueue_impl(R&& input) {
        list_node_t *node = acquire_or_allocate(std::forward<R>(input));
        list_node_t* prev_head = _head.exchange(node, std::memory_order_acq_rel);
        prev_head->next.store(node, std::memory_order_release);
    }

    bool sc_dequeue_impl(T& output) {
        list_node_t* tail = _tail.load(std::memory_order_relaxed);
        list_node_t* next = tail->next.load(std::memory_order_acquire);
        if (!next) return false;
        output = std::move(next->data);
        _tail.store(next, std::memory_order_release);
        freelist_enqueue(tail);
        return true;
    }

    //spin on dequeue contention
    bool mc_dequeue_impl(T& output) {
        list_node_t *tail;
        for (tail = _tail.exchange(nullptr, std::memory_order_acq_rel); !tail; tail = _tail.exchange(nullptr, std::memory_order_acq_rel)) {
            std::this_thread::yield();
        }
        list_node_t *next = tail->next.load(std::memory_order_acquire);
        if (!next) {
            _tail.exchange(tail, std::memory_order_acq_rel);
            return false;
        }
        output = std::move(next->data);
        _tail.store(next, std::memory_order_release);
        freelist_enqueue(tail);
        return true;
    }

    //return false on dequeue contention
    bool mc_dequeue_uncontended_impl(T& output) {
        list_node_t *tail = _tail.exchange(nullptr, std::memory_order_acq_rel);
        if (!tail) return false;
        list_node_t *next = tail->next.load(std::memory_order_acquire);
        if (!next) {
            _tail.exchange(tail, std::memory_order_acq_rel);
            return false;
        }
        output = std::move(next->data);
        _tail.store(next, std::memory_order_release);
        freelist_enqueue(tail);
        return true;
    }

private:

    struct list_node_t {
        T data;
        std::atomic<list_node_t*> next{ nullptr };

        template<typename R>
        list_node_t(R&& input) : data(std::forward<R>(input)) {}
        list_node_t() {}
    };

    struct storage_node_t {
        std::vector<list_node_t> nodes;
        std::atomic<storage_node_t*> next{ nullptr };

        template<typename R>
        storage_node_t(R&& input) : nodes(std::forward<R>(input)) {}
        storage_node_t() {}
    };

    void freelist_enqueue(list_node_t *item) {
        item->next.store(nullptr, std::memory_order_relaxed);
        list_node_t * free_list_prev_head = _free_list_head.exchange(item, std::memory_order_acq_rel);
        free_list_prev_head->next.store(item, std::memory_order_release);
    }

    list_node_t* freelist_try_dequeue() {
        list_node_t *item;
        for (item = _free_list_tail.exchange(nullptr, std::memory_order_acq_rel); !item; item = _free_list_tail.exchange(nullptr, std::memory_order_acq_rel)) {
            std::this_thread::yield();
        }
        list_node_t* next = item->next.load(std::memory_order_acquire);
        if (!next) {
            _free_list_tail.store(item, std::memory_order_release);
            return nullptr;
        }
        _free_list_tail.store(next, std::memory_order_release);
        return item;
    }

    template<typename R>
    list_node_t *acquire_or_allocate(R&& input) {
        //attempt to recycle previously used storage
        list_node_t* node = freelist_try_dequeue();
        if (!node) {
            //pre-allocate for 32 nodes
            static const size_t allocsize = 32;
            std::vector<list_node_t> vec(allocsize);

            //store the sequencing chain here before it goes on the freelist
            for (size_t i = 2; i < allocsize; ++i) {
                vec[i].next.store(&vec[i - 1], std::memory_order_relaxed);
            }

            //connect the chains
            list_node_t* free_list_prev_head = _free_list_head.exchange(&vec[1], std::memory_order_acq_rel);
            free_list_prev_head->next.store(&vec[allocsize - 1], std::memory_order_release);

            //the first one is reserved for this acquire_or_allocate call
            node = &vec[0];

            //store the node on the storage queue for retrieval later
            storage_node_t *store = new storage_node_t(std::move(vec));
            storage_node_t* prev_head = _storage_head.exchange(store, std::memory_order_acq_rel);
            prev_head->next.store(store, std::memory_order_release);

        }
        node->data = std::forward<R>(input);
        return node;
    }

    std::atomic<list_node_t*> _head;
    std::atomic<list_node_t*> _free_list_tail;
    char _padding[64];
    std::atomic<list_node_t*> _tail;
    std::atomic<list_node_t*> _free_list_head;
    std::atomic<storage_node_t*> _storage_head{ new storage_node_t };
    std::atomic<storage_node_t*> _storage_tail{ _storage_head.load(std::memory_order_relaxed) };

};
}//namespace bk_conq

#endif /* BK_CONQ_LISTQUEUE_HPP */
