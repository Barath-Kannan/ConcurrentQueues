/*
 * File:   bounded_list_queue.hpp
 * Author: Barath Kannan
 * This is an bounded multi-producer multi-consumer queue.
 * It can also be used as any combination of single-producer and single-consumer
 * queues for additional performance gains in those contexts. The queue is implemented
 * as a linked list, where nodes are stored in a freelist after being dequeued.
 * Enqueue operations will attempt to acquire items from the freelist or return false
 * if no node is available.
 * Created on 30 January 2017, 08:36 PM
 */

#ifndef BK_CONQ_BOUNDEDLISTQUEUE_HPP
#define BK_CONQ_BOUNDEDLISTQUEUE_HPP

#include <atomic>
#include <thread>
#include <initializer_list>
#include <bk_conq/bounded_queue.hpp>

namespace bk_conq {
template<typename T>
class bounded_list_queue : public bounded_queue<T> {
public:
	bounded_list_queue(size_t N) : _data(N){
		_free_list_head.store(&_data[1], std::memory_order_relaxed);
		_free_list_tail.store(_free_list_head.load(std::memory_order_relaxed));
		for (size_t i = 2; i < N; ++i) {
			list_node_t* tail = &_data[i];
			tail->next.store(_free_list_tail.load(std::memory_order_relaxed));
			_free_list_tail.store(tail);
		}
	}

	virtual ~bounded_list_queue() {
	}

	bounded_list_queue(const bounded_list_queue&) = delete;
	void operator=(const bounded_list_queue&) = delete;

	bool sp_enqueue(T&& input) {
		return sp_enqueue_forward(std::move(input));
	}

	bool sp_enqueue(const T& input) {
		return sp_enqueue_forward(input);
	}

	bool mp_enqueue(T&& input) {
		return mp_enqueue_forward(std::move(input));
	}

	bool mp_enqueue(const T& input) {
		return mp_enqueue_forward(input);
	}

	bool sc_dequeue(T& output) {
		list_node_t* tail = _tail.load(std::memory_order_relaxed);
		list_node_t* next = tail->next.load(std::memory_order_acquire);
		if (!next) return false;
		output = std::move(next->data);
		_tail.store(next, std::memory_order_release);
		freelist_enqueue(tail);
		return true;
	}

	//yield spin on dequeue contention
	bool mc_dequeue(T& output) {
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
	bool mc_dequeue_light(T& output) {
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
		list_node_t(R&& input) : data(input) {}
		list_node_t() {}
	};

	inline void freelist_enqueue(list_node_t *item) {
		item->next.store(nullptr, std::memory_order_relaxed);
		list_node_t * free_list_prev_head = _free_list_head.exchange(item, std::memory_order_acq_rel);
		free_list_prev_head->next.store(item, std::memory_order_release);
	}

	inline list_node_t* freelist_try_dequeue() {
		list_node_t* node = _free_list_tail.load(std::memory_order_relaxed);
		for (list_node_t *next = node->next.load(std::memory_order_acquire); next != nullptr; next = node->next.load(std::memory_order_acquire)) {
			if (_free_list_tail.compare_exchange_strong(node, next)) return node;
		}
		return nullptr;
	}

	template<typename U>
	inline list_node_t *acquire_or_allocate(U&& input) {
		list_node_t *node = freelist_try_dequeue();
		if (!node) {
			node = new list_node_t{ std::forward<U>(input) };
		}
		else {
			node->data = std::forward<U>(input);
			node->next.store(nullptr, std::memory_order_relaxed);
		}
		return node;
	}

	template <typename U>
	bool sp_enqueue_forward(U&& input) {
		list_node_t *node = freelist_try_dequeue();
		if (!node) return false;
		node->data = std::forward<U>(input);
		node->next.store(nullptr, std::memory_order_relaxed);
		_head.load(std::memory_order_relaxed)->next.store(node, std::memory_order_release);
		_head.store(node, std::memory_order_relaxed);
		return true;
	}

	template <typename U>
	bool mp_enqueue_forward(U&& input) {
		list_node_t *node = freelist_try_dequeue();
		if (!node) return false;
		node->data = std::forward<U>(input);
		node->next.store(nullptr, std::memory_order_relaxed);
		list_node_t* prev_head = _head.exchange(node, std::memory_order_acq_rel);
		prev_head->next.store(node, std::memory_order_release);
		return true;
	}

	std::vector<list_node_t>	_data;
	char                        _padding2[64];
	std::atomic<list_node_t*>   _head{ &_data[0] };
	std::atomic<list_node_t*>   _free_list_tail{ nullptr };
	char                        _padding[64];
	std::atomic<list_node_t*>   _tail{ _head.load(std::memory_order_relaxed) };
	std::atomic<list_node_t*>   _free_list_head{ nullptr };
};
}//namespace bk_conq

#endif /* BK_CONQ_BOUNDEDLISTQUEUE_HPP */
