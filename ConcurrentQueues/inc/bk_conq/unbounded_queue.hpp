/*
 * File:   unbounded_queue.hpp
 * Author: Barath Kannan
 * Unbounded queue interface.
 * Created on 17 October 2016, 1:34 PM
 */

#ifndef BK_CONQ_UNBOUNDEDQUEUE_HPP
#define BK_CONQ_UNBOUNDEDQUEUE_HPP

namespace bk_conq {

class unbounded_queue_tag {};

template <typename T>
class unbounded_queue_typed_tag {};

template <typename T, typename BASE>
class unbounded_queue : public unbounded_queue_typed_tag<T>, public unbounded_queue_tag{
public:	
	void sp_enqueue(T&& input) {
		base()->sp_enqueue_impl(std::move(input));
	}

	void sp_enqueue(const T& input) {
		base()->sp_enqueue_impl(input);
	}

	void mp_enqueue(T&& input) {
		base()->mp_enqueue_impl(std::move(input));
	}

	void mp_enqueue(const T& input) {
		base()->mp_enqueue_impl(input);
	}

	bool sc_dequeue(T& output) {
		return base()->sc_dequeue_impl(output);
	}

	bool mc_dequeue(T& output) {
		return base()->mc_dequeue_impl(output);
	}

	bool mc_dequeue_uncontended(T& output) {
		return base()->mc_dequeue_uncontended_impl(output);
	}

private:
	BASE* base() {
		return static_cast<BASE*>(this);
	}
};

}//namespace bk_conq

#endif /* BK_CONQ_UNBOUNDEDQUEUE_HPP */

