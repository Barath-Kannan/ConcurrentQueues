/*
 * File:   multi_bounded_queue.hpp
 * Author: Barath Kannan
 * Vector of bounded list queues. 
 * Created on 28 January 2017, 09:42 AM
 */

#ifndef BK_CONQ_MULTI_BOUNDED_QUEUE_HPP
#define BK_CONQ_MULTI_BOUNDED_QUEUE_HPP

#include <thread>
#include <vector>
#include <numeric>
#include <memory>
#include <bk_conq/bounded_queue.hpp>

namespace bk_conq {
template <typename T>
class multi_bounded_queue : public bounded_queue{
public:

	multi_bounded_queue(size_t N, size_t subqueues){
		static_assert(std::is_base_of<bk_conq::bounded_queue, T>::value, "T must be a bounded queue");
		for (size_t i = 0; i < subqueues; ++i) {
			_q.push_back(std::make_unique<padded_bounded_queue>(N));
		}
	}

	multi_bounded_queue(const multi_bounded_queue&) = delete;
	void operator=(const multi_bounded_queue&) = delete;

	template <typename R>
	bool sp_enqueue(R&& input) {
		thread_local size_t index{ _enqueue_indx.fetch_add(1) % _q.size() };
		return _q[index]->mp_enqueue(std::forward<R>(input));
	}

	template <typename R>
	bool mp_enqueue(R&& input) {
		thread_local size_t index{ _enqueue_indx.fetch_add(1) % _q.size() };
		return _q[index]->mp_enqueue(std::forward<R>(input));
	}

	template <typename R>
	bool sc_dequeue(R& output) {
		thread_local auto hitlist = hitlist_sequence();
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it]->sc_dequeue(output)) {
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
			//this will return false on dequeue contention
			if (_q[*it]->mc_dequeue_light(output)) {
				for (auto it2 = hitlist.begin(); it2 != it; ++it2) std::iter_swap(it, it2);
				return true;
			}
		}
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it]->mc_dequeue(output)) {
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

	class padded_bounded_queue : public T {
	public:
		padded_bounded_queue(size_t N) : T(N) {}
	private:
		char padding[64];
	};

	std::vector<std::unique_ptr<padded_bounded_queue>> _q;
	std::atomic<size_t> _enqueue_indx{ 0 };
};
}//namespace bk_conq

#endif // BK_CONQ_MULTI_BOUNDED_QUEUE_HPP

