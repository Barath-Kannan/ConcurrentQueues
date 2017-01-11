/*
 * File:   cache_queue.hpp
 * Author: Barath Kannan
 * This is a queue implementation that stores items in an array queue and
 * overflows into a list queue. It does not maintain order and can
 * be very unfair to the unbounded list queue at high contention.
 * Created on 24 September 2016, 10:38 PM
 */

#ifndef BK_CONQ_CACHEQUEUE_HPP
#define BK_CONQ_CACHEQUEUE_HPP

#include <bk_conq/array_queue.hpp>
#include <bk_conq/list_queue.hpp>

namespace bk_conq {
template<typename T, size_t CACHE_SIZE>
class cache_queue : public unbounded_queue<T> {
public:

	void sp_enqueue(T&& input) {
		return sp_enqueue_forward(std::move(input));
	}

	void sp_enqueue(const T& input) {
		return sp_enqueue_forward(input);
	}

	void mp_enqueue(T&& input) {
		return mp_enqueue_forward(std::move(input));
	}

	void mp_enqueue(const T& input) {
		return mp_enqueue_forward(input);
	}

	bool sc_dequeue(T& output) {
		if (_aq.mc_dequeue(output)) {
			return true;
		}
		return _lq.mc_dequeue(output);
	}

	bool mc_dequeue(T& output) {
		if (_aq.mc_dequeue(output)) {
			return true;
		}
		return _lq.mc_dequeue(output);
	}

private:
	template <typename U>
	void sp_enqueue_forward(U&& input) {
		if (!_aq.sp_enqueue(std::forward<U>(input))) {
			_lq.sp_enqueue(std::forward<U>(input));
		}
	}

	template <typename U>
	void mp_enqueue_forward(U&& input) {
		if (!_aq.mp_enqueue(std::forward<U>(input))) {
			_lq.mp_enqueue(std::forward<U>(input));
		}
	}

	array_queue<T, CACHE_SIZE>  _aq;
	list_queue<T>               _lq;
};
}//namespace bk_conq

#endif /* BK_CONQ_CACHEQUEUE_HPP */

