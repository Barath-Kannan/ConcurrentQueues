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
template<typename TT>
class multi_bounded_queue;

template <template <typename> class Q, typename T>
class multi_bounded_queue<Q<T>> : public bounded_queue<T, multi_bounded_queue<Q<T>>> {
	friend bounded_queue<T, multi_bounded_queue<Q<T>>>;
public:
	multi_bounded_queue(size_t N, size_t subqueues){
		static_assert(std::is_base_of<bk_conq::bounded_queue_typed_tag<T>, Q<T>>::value, "Q<T> must be a bounded queue");
		for (size_t i = 0; i < subqueues; ++i) {
			_q.push_back(std::make_unique<padded_bounded_queue>(N));
		}
	}

	multi_bounded_queue(const multi_bounded_queue&) = delete;
	void operator=(const multi_bounded_queue&) = delete;

protected:
	struct hitlist_identifier {
		multi_bounded_queue<Q<T>>* pthis;
		std::vector<size_t> hitlist;
	};

	std::vector<size_t>& get_hitlist() {
		thread_local std::vector<hitlist_identifier> hitlist_map;
		thread_local hitlist_identifier* prev = nullptr;
		if (prev && prev->pthis == this) return prev->hitlist;
		for (auto& item : hitlist_map) {
			if (item.pthis == this) {
				prev = &item;
				return item.hitlist;
			}
		}
		hitlist_map.emplace_back(hitlist_identifier{ this, hitlist_sequence() });
		return hitlist_map.back().hitlist;
	}

	struct enqueue_identifier {
		multi_bounded_queue<Q<T>>* pthis;
		size_t index;
	};

	size_t get_enqueue_index() {
		thread_local std::vector<enqueue_identifier> index_map;
		thread_local enqueue_identifier* prev = nullptr;
		if (prev && prev->pthis == this) return prev->index;
		for (auto& item : index_map) {
			if (item.pthis == this) {
				prev = &item;
				return item.index;
			}
		}
		size_t enqueue_index = _enqueue_indx.fetch_add(1) % _q.size();
		index_map.emplace_back(enqueue_identifier{ this, enqueue_index });
		return index_map.back().index;
	}

	template <typename R>
	bool sp_enqueue_impl(R&& input) {
		size_t indx = get_enqueue_index();
		return _q[indx]->mp_enqueue(std::forward<R>(input));
	}

	template <typename R>
	bool mp_enqueue_impl(R&& input) {
		size_t indx = get_enqueue_index();
		return _q[indx]->mp_enqueue(std::forward<R>(input));
	}

	bool sc_dequeue_impl(T& output) {
		auto& hitlist = get_hitlist();
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it]->sc_dequeue(output)) {
				for (auto it2 = hitlist.begin(); it2 != it; ++it2) std::iter_swap(it, it2);
				return true;
			}
		}
		return false;
	}

	bool mc_dequeue_impl(T& output) {
		auto& hitlist = get_hitlist();
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it]->mc_dequeue_uncontended(output)) {
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

	bool mc_dequeue_uncontended_impl(T& output) {
		auto& hitlist = get_hitlist();
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it]->mc_dequeue_light(output)) {
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

	class padded_bounded_queue : public Q<T> {
	public:
		padded_bounded_queue(size_t N) : Q<T>(N) {}
	private:
		char padding[64];
	};

	std::vector<std::unique_ptr<padded_bounded_queue>> _q;
	std::atomic<size_t> _enqueue_indx{ 0 };
};
}//namespace bk_conq

#endif // BK_CONQ_MULTI_BOUNDED_QUEUE_HPP

