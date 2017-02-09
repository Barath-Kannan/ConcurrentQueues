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
#include <mutex>
#include <bk_conq/unbounded_queue.hpp>

namespace bk_conq {

template<typename TT, size_t N=32>
class multi_unbounded_queue;

template <template <typename> class Q, typename T, size_t N>
class multi_unbounded_queue<Q<T>, N> : public unbounded_queue<T, multi_unbounded_queue<Q<T>, N>>{
	friend unbounded_queue<T, multi_unbounded_queue<Q<T>, N>>;
public:
	multi_unbounded_queue(size_t subqueues) :
		_q(subqueues),
		_hitlist([&]() {return hitlist_sequence(); }),
		_enqueue_identifier([&]() { return get_enqueue_index(); }, [&](size_t indx) {return return_enqueue_index(indx); })
	{
		static_assert(std::is_base_of<bk_conq::unbounded_queue_typed_tag<T>, Q<T>>::value, "Q<T> must be an unbounded queue");
	}

	multi_unbounded_queue(const multi_unbounded_queue&) = delete;
	void operator=(const multi_unbounded_queue&) = delete;

protected:
	template <typename R>
	void sp_enqueue_impl(R&& input) {
		size_t indx = _enqueue_identifier.get_tlcl();
		_q[indx].sp_enqueue(std::forward<R>(input));
	}

	template <typename R>
	void mp_enqueue_impl(R&& input) {
		size_t indx = _enqueue_identifier.get_tlcl();
		_q[indx].mp_enqueue(std::forward<R>(input));
	}

	bool sc_dequeue_impl(T& output) {
		auto& hitlist = _hitlist.get_tlcl();
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it].sc_dequeue(output)) {
				for (auto it2 = hitlist.begin(); it2 != it; ++it2) std::iter_swap(it, it2);
				return true;
			}
		}
		return false;
	}

	bool mc_dequeue_impl(T& output) {
		auto& hitlist = _hitlist.get_tlcl();
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
		auto& hitlist = _hitlist.get_tlcl();
		for (auto it = hitlist.begin(); it != hitlist.end(); ++it) {
			if (_q[*it].mc_dequeue_uncontended(output)) {
				for (auto it2 = hitlist.begin(); it2 != it; ++it2) std::iter_swap(it, it2);
				return true;
			}
		}
		return false;
	}

private:

	template <typename U>
	class tlcl {
	public:
		tlcl(std::function<U()> defaultvalfunc = nullptr, std::function<void(U)> defaultdeletefunc = nullptr) :
			_defaultvalfunc(defaultvalfunc),
			_defaultdeletefunc(defaultdeletefunc)
		{
			std::lock_guard<std::mutex> lock(_m);
			if (!_available.empty()) {
				_mycounter = _available.back();
				_available.pop_back();
			}
			else {
				_mycounter = _counter++;
			}
		}

		virtual ~tlcl() {
			std::lock_guard<std::mutex> lock(_m);
			_available.push_back(_mycounter);
		}

		U& get_tlcl() {
			thread_local std::vector<std::pair<tlcl<U>*, U>> vec;
			if (vec.size() <= _mycounter) vec.resize(_mycounter+1);
			auto& ret = vec[_mycounter];
			if (ret.first != this) { //reset to default
				if (_defaultdeletefunc && ret.first != nullptr) _defaultdeletefunc(ret.second);
				if (_defaultvalfunc) ret.second = _defaultvalfunc();
				ret.first = this;
			}
			return ret.second;
		}

	private:
		std::function<U()> _defaultvalfunc;
		std::function<void(U)> _defaultdeletefunc;
		size_t _mycounter;
		static size_t _counter;
		static std::vector<size_t> _available;
		static std::mutex _m;
	};

	std::vector<size_t> hitlist_sequence() {
		std::vector<size_t> hitlist(_q.size());
		std::iota(hitlist.begin(), hitlist.end(), 0);
		return hitlist;
	}

	size_t get_enqueue_index() {
		std::lock_guard<std::mutex> lock(_m);
		if (_unused_enqueue_indexes.empty()) {
			return _enqueue_index++;
		}
		size_t ret = _unused_enqueue_indexes.back();
		_unused_enqueue_indexes.pop_back();
		return ret;
	}

	void return_enqueue_index(size_t index) {
		std::lock_guard<std::mutex> lock(_m);
		_unused_enqueue_indexes.push_back(index);
	}

	class padded_unbounded_queue : public Q<T>{
		char padding[64];
	};

	std::vector<padded_unbounded_queue> _q;
	
	std::vector<size_t> _unused_enqueue_indexes;
	size_t				_enqueue_index{ 0 };
	std::mutex			_m;

	tlcl<std::vector<size_t>> _hitlist;
	tlcl<size_t> _enqueue_identifier;
};

template <template <typename> class Q, typename T, size_t N> template <typename U>
size_t multi_unbounded_queue<Q<T>, N>::tlcl<U>::_counter = 0;

template <template <typename> class Q, typename T, size_t N> template <typename U>
std::vector<size_t> multi_unbounded_queue<Q<T>, N>::tlcl<U>::_available;

template <template <typename> class Q, typename T, size_t N> template <typename U>
std::mutex multi_unbounded_queue<Q<T>, N>::tlcl<U>::_m;

}//namespace bk_conq

#endif /* BK_CONQ_MULTI_UNBOUNDED_QUEUE_HPP */
