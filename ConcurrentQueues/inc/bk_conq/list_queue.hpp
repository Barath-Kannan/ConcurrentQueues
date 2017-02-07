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
#include <bk_conq/unbounded_queue.hpp>

namespace bk_conq {

template<typename T>
class list_queue : public unbounded_queue<T, list_queue<T>> {
	friend unbounded_queue<T, list_queue<T>>;
	static const size_t BLOCK_SIZE = 1024;
	static const size_t BLOCKS_ALLOCATED = 8;
public:
	list_queue() {
		std::vector<list_node_t> vec(3);
		_head.store(&vec[0]);
		_tail.store(_head.load(std::memory_order_relaxed), std::memory_order_relaxed);
		_free_list_head.store(&vec[1]);
		_free_list_tail.store(_free_list_head.load(std::memory_order_relaxed), std::memory_order_relaxed);
		_in_progress_head.store(&vec[2]);
		_in_progress_tail.store(_in_progress_head.load(std::memory_order_relaxed), std::memory_order_relaxed);
		storage_node_t *store = new storage_node_t(std::move(vec));
		storage_node_t* prev_head = _storage_head.exchange(store, std::memory_order_acq_rel);
		prev_head->next.store(store, std::memory_order_release);
	}

	virtual ~list_queue() {
		storage_node_t* tail = _storage_tail.load(std::memory_order_relaxed);
		storage_node_t* next = tail->next.load(std::memory_order_relaxed);
		for(storage_node_t* next = tail->next.load(std::memory_order_relaxed); next != nullptr; ) {
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
		if (node->indx != node->data.size()) {
			inprogress_enqueue(node);
			return;
		}
		_head.load(std::memory_order_relaxed)->next.store(node, std::memory_order_release);
		_head.store(node, std::memory_order_relaxed);
	}

	template <typename R>
	void mp_enqueue_impl(R&& input){
		list_node_t *node = acquire_or_allocate(std::forward<R>(input));
		if (node->indx != node->data.size()) {
			inprogress_enqueue(node);
			return;
		}
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

	bool dequeue_common(list_node_t* tail, T& output) {
		//get item directly from tail
		if (tail->indx != 0) { //success
			output = std::move(tail->data[--tail->indx]);
			if (tail->indx == 0) { //if we are now empty
				list_node_t *next = tail->next.load(std::memory_order_acquire);
				if (next) { //we only want to progress if tail->next is valid
					_tail.store(next, std::memory_order_release);
					freelist_enqueue(tail);
				}
			}
			else { //more elements, just put it back
				_tail.store(tail, std::memory_order_release); 
			}
			return true;
		}
		
		//nothing in tail, go next
		list_node_t *next = tail->next.load(std::memory_order_acquire);
		if (!next) { //restore the tail so that other dequeue operations can progress
			_tail.store(tail, std::memory_order_release);
			return try_get_from_inprogress(output);
		}
		output = std::move(next->data[--next->indx]);
		if (next->indx == 0) {
			_tail.store(next, std::memory_order_release);
			freelist_enqueue(tail);
		}
		else {
			_tail.store(tail, std::memory_order_release);
		}
		return true;
	}

	void freelist_enqueue(list_node_t *item) {
		item->next.store(nullptr, std::memory_order_relaxed);
		list_node_t * free_list_prev_head = _free_list_head.exchange(item, std::memory_order_acq_rel);
		free_list_prev_head->next.store(item, std::memory_order_release);
	}

	list_node_t* freelist_try_dequeue() {
		list_node_t *tail;
		for (tail = _free_list_tail.exchange(nullptr, std::memory_order_acq_rel); !tail; tail = _free_list_tail.exchange(nullptr, std::memory_order_acq_rel)) {
			std::this_thread::yield();
		}
		list_node_t *next = tail->next.load(std::memory_order_acquire);
		if (!next) {
			_free_list_tail.store(tail, std::memory_order_release);
			return false;
		}
		_free_list_tail.store(next, std::memory_order_release);
		return tail;
	}

	void inprogress_enqueue(list_node_t *item) {
		item->next.store(nullptr, std::memory_order_relaxed);
		list_node_t * inprog_prev_head = _in_progress_head.exchange(item, std::memory_order_acq_rel);
		inprog_prev_head->next.store(item, std::memory_order_release);
	}

	list_node_t* inprogress_try_dequeue() {
		list_node_t *tail;
		for (tail = _in_progress_tail.exchange(nullptr, std::memory_order_acq_rel); !tail; tail = _in_progress_tail.exchange(nullptr, std::memory_order_acq_rel)) {
			std::this_thread::yield();
		}
		list_node_t *next = tail->next.load(std::memory_order_acquire);
		if (!next) {
			_in_progress_tail.store(tail, std::memory_order_release);
			return nullptr;
		}
		_in_progress_tail.store(next, std::memory_order_release);
		return tail;
	}

	bool try_get_from_inprogress(T& output) {
		list_node_t *tail;
		for (tail = _in_progress_tail.exchange(nullptr, std::memory_order_acq_rel); !tail; tail = _in_progress_tail.exchange(nullptr, std::memory_order_acq_rel)) {
			std::this_thread::yield();
		}
		if (tail->indx != 0) {
			output = std::move(tail->data[--tail->indx]);
			if (tail->indx == 0) { //if we are now empty
				list_node_t *next = tail->next.load(std::memory_order_acquire);
				if (next) { //we only want to progress if tail->next is valid
					_in_progress_tail.store(next, std::memory_order_release);
					freelist_enqueue(tail);
					return true;
				}
			}
			//either more elements or next node is nullptr so can't progress
			_in_progress_tail.store(tail, std::memory_order_release);
			return true;

		}
		_in_progress_tail.store(tail, std::memory_order_release);
		return false;
	}
	
	template<typename R>
	list_node_t *acquire_or_allocate(R&& input) {
		//try get space from the in progress enqueue operations
		list_node_t* node = inprogress_try_dequeue();
		if (!node) { //attempt to recycle previously used storage
			node = freelist_try_dequeue();
		}
		if (!node) { //allocate new storage
			std::vector<list_node_t> vec(BLOCKS_ALLOCATED);

			//store the sequencing chain here before it goes on the freelist
			for (size_t i = 2; i < BLOCKS_ALLOCATED; ++i) {
				vec[i].next.store(&vec[i - 1], std::memory_order_relaxed);
			}

			//connect the chains
			list_node_t* free_list_prev_head = _free_list_head.exchange(&vec[1], std::memory_order_acq_rel);
			free_list_prev_head->next.store(&vec[BLOCKS_ALLOCATED - 1], std::memory_order_release);

			//the first one is reserved for this acquire_or_allocate call
			node = &vec[0];

			//store the node on the storage queue for retrieval later
			storage_node_t *store = new storage_node_t(std::move(vec));
			storage_node_t* prev_head = _storage_head.exchange(store, std::memory_order_acq_rel);
			prev_head->next.store(store, std::memory_order_release);
		}
		node->data[node->indx++] = std::forward<R>(input);
		node->next.store(nullptr, std::memory_order_release);
		return node;
	}

	std::atomic<list_node_t*>   _head;
	std::atomic<list_node_t*>   _free_list_tail;
	char                        _padding[64];
	std::atomic<list_node_t*>   _tail;
	std::atomic<list_node_t*>   _free_list_head;
	char						_padding2[64];
	std::atomic<list_node_t*>   _in_progress_head;
	std::atomic<storage_node_t*> _storage_head{ new storage_node_t };
	std::atomic<list_node_t*>   _in_progress_tail;
	std::atomic<storage_node_t*> _storage_tail{ _storage_head.load(std::memory_order_relaxed) };
	

};
}//namespace bk_conq

#endif /* BK_CONQ_LISTQUEUE_HPP */
