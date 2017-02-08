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
		if (!node) return;
		_head.load(std::memory_order_relaxed)->next.store(node, std::memory_order_release);
		_head.store(node, std::memory_order_relaxed);
	}

	template <typename R>
	void mp_enqueue_impl(R&& input){
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

	inline void list_enqueue(list_node_t* item, std::atomic<list_node_t*>& head) {
		item->next.store(nullptr, std::memory_order_relaxed);
		list_node_t * prev_head = head.exchange(item, std::memory_order_acq_rel);
		prev_head->next.store(item, std::memory_order_release);
	}

	inline list_node_t* list_dequeue(std::atomic<list_node_t*>& tail) {
		list_node_t *item;
		for (item = tail.exchange(nullptr, std::memory_order_acq_rel); !item; item = tail.exchange(nullptr, std::memory_order_acq_rel)) {
			std::this_thread::yield();
		}
		list_node_t *next = item->next.load(std::memory_order_acquire);
		if (!next) {
			tail.store(item, std::memory_order_release);
			return false;
		}
		tail.store(next, std::memory_order_release);
		return item;
	}

	template <bool store_on_failure=true>
	inline bool list_take(T& output, list_node_t* item, std::atomic<list_node_t*>& list) {
		if (item->indx != 0) { //success
			output = std::move(item->data[--item->indx]);
			if (item->indx == 0) { //if we are now empty
				list_node_t *next = item->next.load(std::memory_order_acquire);
				if (next) { //we only want to progress if tail->next is valid
					list.store(next, std::memory_order_release);
					freelist_enqueue(item);
					return true;
				}
			}
			//either more elements or next node is nullptr so can't progress
			list.store(item, std::memory_order_release);
			return true;
		}
		if (store_on_failure) {
			list.store(item, std::memory_order_release);
		}
		return false;
	}

	bool dequeue_common(list_node_t* tail, T& output) {
		//get item directly from tail
		if (list_take<false>(output, tail, _tail)) return true;
		
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

	bool try_get_from_inprogress(T& output) {
		list_node_t *item;
		for (item = _in_progress_tail.exchange(nullptr, std::memory_order_acq_rel); !item; item = _in_progress_tail.exchange(nullptr, std::memory_order_acq_rel)) {
			std::this_thread::yield();
		}
		return list_take(output, item, _in_progress_tail);
	}

	
	inline list_node_t* allocate() {
		std::vector<list_node_t> vec(BLOCKS_ALLOCATED);

		//store the sequencing chain here before it goes on the freelist
		for (size_t i = 2; i < BLOCKS_ALLOCATED; ++i) {
			vec[i].next.store(&vec[i - 1], std::memory_order_relaxed);
		}

		//connect the chains
		list_node_t* free_list_prev_head = _free_list_head.exchange(&vec[1], std::memory_order_acq_rel);
		free_list_prev_head->next.store(&vec[BLOCKS_ALLOCATED - 1], std::memory_order_release);

		//the first one is reserved for this acquire_or_allocate call
		auto node = &vec[0];

		//store the node on the storage queue for retrieval later
		storage_node_t *store = new storage_node_t(std::move(vec));
		storage_node_t* prev_head = _storage_head.exchange(store, std::memory_order_acq_rel);
		prev_head->next.store(store, std::memory_order_release);
		
		return node;
	}

	template<typename R>
	list_node_t *acquire_or_allocate(R&& input) {
		list_node_t* node = inprogress_try_dequeue(); //try get space from the in progress enqueue operations
		if (!node) node = freelist_try_dequeue();//attempt to recycle previously used storage 
		if (!node) node = allocate(); //allocate new storage
		node->data[node->indx++] = std::forward<R>(input);
		node->next.store(nullptr, std::memory_order_release);
		if (node->indx != node->data.size()) {
			inprogress_enqueue(node);
			return nullptr;
		}
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
