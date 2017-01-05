/* 
 * File:   blocking_unbounded_queue.hpp
 * Author: Barath Kannan
 * Template for a blocking unbounded producer consumer queue
 * Created on 17 October 2016, 1:34 PM
 */

#ifndef BK_CONQ_BLOCKINGUNBOUNDEDQUEUE_HPP
#define BK_CONQ_BLOCKINGUNBOUNDEDQUEUE_HPP

#include <bk_conq/unbounded_queue.hpp>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace bk_conq{
template<typename T>
class blocking_unbounded_queue : private T{
public:

	blocking_unbounded_queue() : T() {}

	blocking_unbounded_queue(size_t subqueues) : T(subqueues) {}

    virtual ~blocking_unbounded_queue(){};
    
	template <typename R>
	void sp_enqueue(R&& input) {
		static_assert(std::is_base_of<bk_conq::unbounded_queue<std::decay<R>::type>, T>::value, "T must be an unbounded queue");
		T::sp_enqueue(std::forward<R>(input));
		if (waitingReaders.load(std::memory_order_acquire)) _cv.notify_one();
	}

	template <typename R>
	void mp_enqueue(R&& input) {
		static_assert(std::is_base_of<bk_conq::unbounded_queue<std::decay<R>::type>, T>::value, "T must be an unbounded queue");
		T::mp_enqueue(std::forward<R>(input));
		if (waitingReaders.load(std::memory_order_acquire)) _cv.notify_one();
	}
    
	template <typename R>
	bool try_sc_dequeue(R& output) {
		static_assert(std::is_base_of<bk_conq::unbounded_queue<R>, T>::value, "T must be an unbounded queue");
		if (T::sc_dequeue(output)) return;
	}

	template <typename R>
	void sc_dequeue(R& output) {
		static_assert(std::is_base_of<bk_conq::unbounded_queue<R>, T>::value, "T must be an unbounded queue");
		if (T::sc_dequeue(output)) return;
		waitingReaders.fetch_add(1, std::memory_order_release);
		std::unique_lock<std::mutex> lock(_m);
		while (!T::sc_dequeue(output)){
		    _cv.wait(lock);
		}
	}

	template <typename R>
	bool try_mc_dequeue(R& output) {
		static_assert(std::is_base_of<bk_conq::unbounded_queue<R>, T>::value, "T must be an unbounded queue");
		if (T::mc_dequeue(output)) return;
	}
    
	template <typename R>
	void mc_dequeue(R& output) {
		static_assert(std::is_base_of<bk_conq::unbounded_queue<R>, T>::value, "T must be an unbounded queue");
		if (T::mc_dequeue(output)) return;
		waitingReaders.fetch_add(1, std::memory_order_release);
		std::unique_lock<std::mutex> lock(_m);
		while (!T::mc_dequeue(output)) {
			_cv.wait(lock);
		}
	}

private:
	std::mutex _m;
	std::condition_variable _cv;
    std::atomic<size_t> waitingReaders{0};
};
}//namespace bk_conq

#endif /* BK_CONQ_BLOCKINGUNBOUNDEDQUEUE_HPP */

