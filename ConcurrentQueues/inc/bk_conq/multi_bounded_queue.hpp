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
#include <mutex>
#include <memory>
#include <numeric>
#include <bk_conq/bounded_queue.hpp>
#include <bk_conq/details/tlos.hpp>

namespace bk_conq {
template<typename TT>
class multi_bounded_queue;

template <template <typename> class Q, typename T>
class multi_bounded_queue<Q<T>> : public bounded_queue<T, multi_bounded_queue<Q<T>>> {
	friend bounded_queue<T, multi_bounded_queue<Q<T>>>;
public:
	multi_bounded_queue(size_t N, size_t subqueues) : 
		_hitlist([&]() {return hitlist_sequence(); }),
		_enqueue_identifier([&]() { return get_enqueue_index(); }, [&](size_t indx) {return return_enqueue_index(indx); })
	{
		static_assert(std::is_base_of<bk_conq::bounded_queue_typed_tag<T>, Q<T>>::value, "Q<T> must be a bounded queue");
		for (size_t i = 0; i < subqueues; ++i) {
			_q.push_back(std::make_unique<padded_bounded_queue>(N));
		}
	}

	multi_bounded_queue(const multi_bounded_queue&) = delete;
	void operator=(const multi_bounded_queue&) = delete;

protected:
	template <typename R>
	bool sp_enqueue_impl(R&& input) {
		size_t indx = _enqueue_identifier.get();
		return _q[indx]->mp_enqueue(std::forward<R>(input));
	}

	template <typename R>
	bool mp_enqueue_impl(R&& input) {
		size_t indx = _enqueue_identifier.get();
		return _q[indx]->mp_enqueue(std::forward<R>(input));
	}

	bool sc_dequeue_impl(T& output) {
		auto& hitlist = _hitlist.get();
		for (auto it = hitlist.cbegin(); it != hitlist.cend(); ++it) {
			if (_q[*it]->sc_dequeue(output)) {
				if (hitlist.cbegin() == it) return true;
				//as below
				auto nonconstit = hitlist.erase(it, it);
				for (auto it2 = hitlist.begin(); it2 != nonconstit; ++it2) std::iter_swap(nonconstit, it2);
				return true;
			}
		}
		return false;
	}

	bool mc_dequeue_impl(T& output) {
		auto& hitlist = _hitlist.get();
		for (auto it = hitlist.cbegin(); it != hitlist.cend(); ++it) {
			if (_q[*it]->mc_dequeue_uncontended(output)) {
				if (hitlist.cbegin() == it) return true;
				//funky magic - range erase returns an iterator, but an empty range is provided so contents aren't changed
				//this converts a const iterator to an iterator in constant time
				auto nonconstit = hitlist.erase(it, it);
				for (auto it2 = hitlist.begin(); it2 != nonconstit; ++it2) std::iter_swap(nonconstit, it2);
				return true;
			}
		}
		for (auto it = hitlist.cbegin(); it != hitlist.cend(); ++it) {
			if (_q[*it]->mc_dequeue(output)) {
				if (hitlist.cbegin() == it) return true;
				//as above
				auto nonconstit = hitlist.erase(it, it);
				for (auto it2 = hitlist.begin(); it2 != nonconstit; ++it2) std::iter_swap(nonconstit, it2);
				return true;
			}
		}
		return false;
	}

	bool mc_dequeue_uncontended_impl(T& output) {
		auto& hitlist = _hitlist.get();
		for (auto it = hitlist.cbegin(); it != hitlist.cend(); ++it) {
			if (_q[*it]->mc_dequeue_uncontended(output)) {
				if (hitlist.cbegin() == it) return true;
				//as above
				auto nonconstit = hitlist.erase(it, it);
				for (auto it2 = hitlist.begin(); it2 != nonconstit; ++it2) std::iter_swap(nonconstit, it2);
				return true;
			}
		}
		return false;
	}

private:
    class padded_bounded_queue : public Q<T> {
    public:
        padded_bounded_queue(size_t N) : Q<T>(N) {}
    private:
        char padding[64];
    };

	std::vector<size_t> hitlist_sequence() {
		std::vector<size_t> hitlist(_q.size());
		std::iota(hitlist.begin(), hitlist.end(), 0);
		return hitlist;
	}

	size_t get_enqueue_index() {
		std::lock_guard<std::mutex> lock(_m);
		if (_unused_enqueue_indexes.empty()) {
			return (_enqueue_index++) % _q.size();
		}
		size_t ret = _unused_enqueue_indexes.back();
		_unused_enqueue_indexes.pop_back();
		return ret;
	}

	void return_enqueue_index(size_t index) {
		std::lock_guard<std::mutex> lock(_m);
		_unused_enqueue_indexes.push_back(index);
	}

	std::vector<size_t> _unused_enqueue_indexes;
	std::vector<std::unique_ptr<padded_bounded_queue>> _q;
	size_t				_enqueue_index{ 0 };
	std::mutex			_m;
	details::tlos<std::vector<size_t>, multi_bounded_queue<Q<T>>> _hitlist;
	details::tlos<size_t, multi_bounded_queue<Q<T>>> _enqueue_identifier;
};

}//namespace bk_conq

#endif // BK_CONQ_MULTI_BOUNDED_QUEUE_HPP

