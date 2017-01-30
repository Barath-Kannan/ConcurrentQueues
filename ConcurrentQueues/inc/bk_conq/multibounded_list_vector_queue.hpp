/*
 * File:   multibounded_list_vector_queue.hpp
 * Author: Barath Kannan
 * Vector of bounded list queues. 
 * Created on 28 January 2017, 09:42 AM
 */

#ifndef BK_CONQ_MULTIBOUNDED_LIST_VECTOR_QUEUE_HPP
#define BK_CONQ_MULTIBOUNDED_LIST_VECTOR_QUEUE_HPP

#include <thread>
#include <vector>
#include <numeric>
#include <memory>
#include <bk_conq/bounded_queue.hpp>
#include <bk_conq/bounded_list_queue.hpp>

namespace bk_conq {
template <typename T>
class multibounded_list_vector_queue : public bounded_queue<T> {
public:
	multibounded_list_vector_queue(size_t N, size_t subqueues){
		for (size_t i = 0; i < subqueues; ++i) {
			_q.push_back(std::make_unique<padded_bounded_list_queue>(N));
		}
	}

	multibounded_list_vector_queue(const multibounded_list_vector_queue&) = delete;
	void operator=(const multibounded_list_vector_queue&) = delete;

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
		thread_local static auto hitlist = hitlist_sequence();
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it]->sc_dequeue(output)) {
				for (auto it2 = hitlist.begin(); it2 != it; ++it2) std::iter_swap(it, it2);
				return true;
			}
		}
		return false;
	}

	bool mc_dequeue(T& output) {
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

	template <typename U>
	bool sp_enqueue_forward(U&& input) {
		thread_local static size_t index{ _enqueue_indx.fetch_add(1) % _q.size() };
		return _q[index]->mp_enqueue(std::forward<U>(input));
	}

	template <typename U>
	bool sp_enqueue_forward(U&& input, size_t index) {
		return _q[index]->mp_enqueue(std::forward<U>(input));
	}

	template <typename U>
	bool mp_enqueue_forward(U&& input) {
		thread_local static size_t index{ _enqueue_indx.fetch_add(1) % _q.size() };
		return _q[index]->mp_enqueue(std::forward<U>(input));
	}

	template <typename U>
	bool mp_enqueue_forward(U&& input, size_t index) {
		return _q[index]->mp_enqueue(std::forward<U>(input));
	}

	class padded_bounded_list_queue : public bounded_list_queue<T> {
	public:
		padded_bounded_list_queue(size_t N) : bounded_list_queue<T>(N) {}
	private:
		char padding[64];
	};

	std::vector<std::unique_ptr<padded_bounded_list_queue>> _q;
	std::atomic<size_t> _enqueue_indx{ 0 };
};
}//namespace bk_conq

#endif // BK_CONQ_MULTIBOUNDED_LIST_VECTOR_QUEUE_HPP

