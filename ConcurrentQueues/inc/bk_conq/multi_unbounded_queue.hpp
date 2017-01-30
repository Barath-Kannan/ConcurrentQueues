/*
 * File:   multi_unbounded_queue.hpp
 * Author: Barath Kannan
 * Vector of unbounded queues. Enqueue operations are assigned a subqueue,
 * which is used for all enqueue operations occuring from that thread. The dequeue
 * operation maintains a list of subqueues on which a "hit" has occured - pertaining
 * to the subqueues from which a successful dequeue operation has occured. On a
 * successful dequeue operation, the queue that is used is pushed to the front of the
 * list. The "hit lists" allow the queue to adapt fairly well to different usage contexts.
 * Created on 25 September 2016, 12:04 AM
 */

#ifndef BK_CONQ_MULTI_UNBOUNDED_QUEUE_HPP
#define BK_CONQ_MULTI_UNBOUNDED_QUEUE_HPP

#include <thread>
#include <vector>
#include <numeric>
#include <bk_conq/unbounded_queue.hpp>

namespace bk_conq {
template <typename T>
class multi_unbounded_queue : public unbounded_queue {
public:
	multi_unbounded_queue(size_t subqueues) : _q(subqueues) {
		static_assert(std::is_base_of<bk_conq::unbounded_queue, T>::value, "T must be an unbounded queue");
	}

	multi_unbounded_queue(const multi_unbounded_queue&) = delete;
	void operator=(const multi_unbounded_queue&) = delete;

	template <typename R>
	void sp_enqueue(R&& input) {
		thread_local size_t indx{ _enqueue_indx.fetch_add(1) % _q.size() };
		_q[indx].mp_enqueue(std::forward<R>(input));
	}

	template <typename R>
	void mp_enqueue(R&& input) {
		thread_local size_t indx{ _enqueue_indx.fetch_add(1) % _q.size() };
		_q[indx].mp_enqueue(std::forward<R>(input));
	}

	template <typename R>
	bool sc_dequeue(R& output) {
		thread_local auto hitlist = hitlist_sequence();
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it].sc_dequeue(output)) {
				for (auto it2 = hitlist.begin(); it2 != it; ++it2) std::iter_swap(it, it2);
				return true;
			}
		}
		return false;
	}

	template <typename R>
	bool mc_dequeue(R& output) {
		thread_local auto hitlist = hitlist_sequence();
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it].mc_dequeue_light(output)) {
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
	std::vector<size_t> hitlist_sequence() {
		std::vector<size_t> hitlist(_q.size());
		std::iota(hitlist.begin(), hitlist.end(), 0);
		return hitlist;
	}

	class padded_unbounded_queue : public T{
		char padding[64];
	};

	std::vector<padded_unbounded_queue> _q;
	std::atomic<size_t> _enqueue_indx{ 0 };
};
}//namespace bk_conq

#endif /* BK_CONQ_MULTI_UNBOUNDED_QUEUE_HPP */
