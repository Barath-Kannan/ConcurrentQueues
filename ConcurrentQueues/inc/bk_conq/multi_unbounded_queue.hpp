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
#include <bk_conq/unbounded_queue.hpp>

namespace bk_conq {

template<typename TT, size_t N>
class multi_unbounded_queue;

template <template <typename> class Q, typename T, size_t N>
class multi_unbounded_queue<Q<T>, N> : public unbounded_queue<T, multi_unbounded_queue<Q<T>, N>>{
	friend unbounded_queue<T, multi_unbounded_queue<Q<T>, N>>;
public:
	multi_unbounded_queue(size_t subqueues) : _q(subqueues) {
		static_assert(std::is_base_of<bk_conq::unbounded_queue_typed_tag<T>, Q<T>>::value, "Q<T> must be an unbounded queue");
	}

	multi_unbounded_queue(const multi_unbounded_queue&) = delete;
	void operator=(const multi_unbounded_queue&) = delete;

protected:
	template <typename R>
	void sp_enqueue_impl(R&& input) {
		thread_local size_t indx = _enqueue_indx.fetch_add(1) % _q.size();
		_q[indx].sp_enqueue(std::forward<R>(input));
	}

	template <typename R>
	void mp_enqueue_impl(R&& input) {
		thread_local size_t indx = _enqueue_indx.fetch_add(1) % _q.size();
		_q[indx].mp_enqueue(std::forward<R>(input));
	}

	bool sc_dequeue_impl(T& output) {
		thread_local std::vector<size_t> hitlist = hitlist_sequence();
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it].sc_dequeue(output)) {
				for (auto it2 = hitlist.begin(); it2 != it; ++it2) std::iter_swap(it, it2);
				return true;
			}
		}
		return false;
	}

	bool mc_dequeue_impl(T& output) {
		thread_local std::vector<size_t> hitlist = hitlist_sequence();
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it].mc_dequeue_uncontended(output)) {
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

	bool mc_dequeue_uncontended_impl(T& output) {
		thread_local std::vector<size_t> hitlist = hitlist_sequence();
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it].mc_dequeue_uncontended(output)) {
				for (auto it2 = hitlist.begin(); it2 != it; ++it2) std::iter_swap(it, it2);
				return true;
			}
		}
		return false;
	}

private:
	inline std::vector<size_t> hitlist_sequence() {
		std::vector<size_t> hitlist(_q.size());
		std::iota(hitlist.begin(), hitlist.end(), 0);
		return hitlist;
	}

	class padded_unbounded_queue : public Q<T>{
		char padding[64];
	};

	std::vector<padded_unbounded_queue> _q;
	std::atomic<size_t> _enqueue_indx{ 0 };
};

}//namespace bk_conq

#endif /* BK_CONQ_MULTI_UNBOUNDED_QUEUE_HPP */
