/*
 * File:   multiarray_queue.hpp
 * Author: Barath Kannan
 * Array of bounded array queues. Enqueue operations are assigned a subqueue,
 * which is used for all enqueue operations occuring from that thread. The deque
 * operation maintains a list of subqueues on which a "hit" has occured - pertaining
 * to the subqueues from which a successful dequeue operation has occured. On a
 * successful dequeue operation, the queue that is used is pushed to the front of the
 * list. The "hit lists" allow the queue to adapt fairly well to different usage contexts,
 * including when there are more readers than writers, more writers than readers,
 * and high contention.
 * Created on 28 January 2017, 09:27 AM
 */

#ifndef BK_CONQ_MULTIARRAYQUEUE_HPP
#define BK_CONQ_MULTIARRAYQUEUE_HPP

#include <thread>
#include <array>
#include <numeric>
#include <bk_conq/array_queue.hpp>

namespace bk_conq {
template <typename T, size_t N, size_t SUBQUEUES>
class multiarray_queue : public bounded_queue<T> {
public:
	multiarray_queue() {}
	multiarray_queue(const multiarray_queue&) = delete;
	void operator=(const multiarray_queue&) = delete;

	bool sp_enqueue(T&& input) {
		return sp_enqueue_forward(std::move(input));
	}

	bool sp_enqueue(const T& input) {
		return sp_enqueue_forward(input);
	}

	bool sp_enqueue(T&& input, size_t index) {
		return sp_enqueue_forward(std::move(input), index);
	}

	bool sp_enqueue(const T& input, size_t index) {
		return sp_enqueue_forward(std::move(input), index);
	}

	bool mp_enqueue(T&& input) {
		return mp_enqueue_forward(std::move(input));
	}

	bool mp_enqueue(const T& input) {
		return mp_enqueue_forward(input);
	}

	bool mp_enqueue(T&& input, size_t index) {
		return mp_enqueue_forward(std::move(input), index);
	}

	bool mp_enqueue(const T& input, size_t index) {
		return mp_enqueue_forward(input, index);
	}

	bool sc_dequeue(T& output) {
		thread_local static std::array<size_t, SUBQUEUES> hitlist = hitlist_sequence();
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it].sc_dequeue(output)) {
				for (auto it2 = hitlist.begin(); it2 != it; ++it2) std::iter_swap(it, it2);
				return true;
			}
		}
		return false;
	}

	bool mc_dequeue(T& output) {
		thread_local static std::array<size_t, SUBQUEUES> hitlist = hitlist_sequence();
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			//this will return false on dequeue contention in an array_queue
			if (_q[*it].sc_dequeue(output)) {
				for (auto it2 = hitlist.begin(); it2 != it; ++it2) std::iter_swap(it, it2);
				return true;
			}
		}
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it].mc_dequeue(output)) {
				for (auto it2 = hitlist.begin(); it2 != it; ++it2) std::iter_swap(it, it2);
				return true;
			}
		}
		return false;
	}

private:
	std::array<size_t, SUBQUEUES> hitlist_sequence() {
		std::array<size_t, SUBQUEUES> hitlist;
		std::iota(hitlist.begin(), hitlist.end(), 0);
		return hitlist;
	}

	template <typename U>
	bool sp_enqueue_forward(U&& input) {
		thread_local static size_t indx{ _enqueue_indx.fetch_add(1) % SUBQUEUES };
		return _q[indx].mp_enqueue(std::forward<U>(input));
	}

	template <typename U>
	bool sp_enqueue_forward(U&& input, size_t index) {
		return _q[index].mp_enqueue(std::forward<U>(input));
	}

	template <typename U>
	bool mp_enqueue_forward(U&& input) {
		thread_local static size_t indx{ _enqueue_indx.fetch_add(1) % SUBQUEUES };
		return _q[indx].mp_enqueue(std::forward<U>(input));
	}

	template <typename U>
	bool mp_enqueue_forward(U&& input, size_t index) {
		return _q[index].mp_enqueue(std::forward<U>(input));
	}

	class padded_array_queue : public array_queue<T, N> {
		char padding[64];
	};

	std::array<padded_array_queue, SUBQUEUES> _q;
	std::atomic<size_t> _enqueue_indx{ 0 };
};
}//namespace bk_conq

#endif /* BK_CONQ_MULTIARRAYQUEUE_HPP */

