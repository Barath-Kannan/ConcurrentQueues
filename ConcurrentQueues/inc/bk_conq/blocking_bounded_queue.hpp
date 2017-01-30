/*
 * File:   blocking_bounded_queue.hpp
 * Author: Barath Kannan
 * Template for a blocking unbounded producer consumer queue
 * Created on 10 January 2017 9:21 PM
 */

#ifndef BK_CONQ_BLOCKINGBOUNDEDQUEUE_HPP
#define BK_CONQ_BLOCKINGBOUNDEDQUEUE_HPP

#include <bk_conq/bounded_queue.hpp>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <type_traits>

namespace bk_conq {
template<typename T>
class blocking_bounded_queue : private T {
public:

	template <typename... Args>
	blocking_bounded_queue(Args&&... args) : T(args...) {
		static_assert(std::is_base_of<bk_conq::bounded_queue, T>::value, "T must be a bounded queue");
	}

	virtual ~blocking_bounded_queue() {};

	template <typename R>
	bool try_sp_enqueue(R&& input) {
		if (T::sp_enqueue(std::forward<R>(input))) {
			_cv.notify_one();
			return true;
		}
		return false;
	}

	template <typename R>
	void sp_enqueue(R&& input) {
		if (!T::sp_enqueue(std::forward<R>(input))) {
			std::unique_lock<std::mutex> lock(_m2);
			while (!T::sp_enqueue(std::forward<R>(input))) {
				_cv2.wait(lock);
			}
		}
		_cv.notify_one();
	}

	template <typename R>
	bool try_mp_enqueue(R&& input) {
		if (T::mp_enqueue(std::forward<R>(input))) {
			_cv.notify_one();
			return true;
		}
		return false;
	}

	template <typename R>
	void mp_enqueue(R&& input) {
		if (!T::mp_enqueue(std::forward<R>(input))) {
			std::unique_lock<std::mutex> lock(_m2);
			while (!T::mp_enqueue(std::forward<R>(input))) {
				_cv2.wait(lock);
			}
		}
		_cv.notify_one();
	}

	template <typename R>
	bool try_sc_dequeue(R& output) {
		if (T::sc_dequeue(output)) {
			_cv2.notify_one();
			return true;
		}
		return false;
	}

	template <typename R>
	void sc_dequeue(R& output) {
		if (!T::sc_dequeue(output)) {
			std::unique_lock<std::mutex> lock(_m);
			while (!T::sc_dequeue(output)) {
				_cv.wait(lock);
			}
		}
		_cv2.notify_one();
	}

	template <typename R>
	bool try_mc_dequeue(R& output) {
		if (T::mc_dequeue(output)) {
			_cv2.notify_one();
			return true;
		}
		return false;
	}

	template <typename R>
	void mc_dequeue(R& output) {
		if (!T::mc_dequeue(output)) {
			std::unique_lock<std::mutex> lock(_m);
			while (!T::mc_dequeue(output)) {
				_cv.wait(lock);
			}
		}
		_cv2.notify_one();
	}

private:
	std::mutex _m;
	std::condition_variable _cv;
	std::mutex _m2;
	std::condition_variable _cv2;
};
}//namespace bk_conq

#endif /* BK_CONQ_BLOCKINGBOUNDEDQUEUE_HPP */

