/*
 * File:   unbounded_queue.hpp
 * Author: Barath Kannan
 * Abstract specification for an unbounded producer consumer queue
 * Created on 17 October 2016, 1:34 PM
 */

#ifndef BK_CONQ_UNBOUNDEDQUEUE_HPP
#define BK_CONQ_UNBOUNDEDQUEUE_HPP

namespace bk_conq {
template<typename T>
class unbounded_queue {
public:
	virtual ~unbounded_queue() {};

	virtual void sp_enqueue(T&& input) = 0;
	virtual void sp_enqueue(const T& input) = 0;
	virtual void mp_enqueue(T&& input) = 0;
	virtual void mp_enqueue(const T& input) = 0;

	virtual bool sc_dequeue(T& output) = 0;
	virtual bool mc_dequeue(T& output) = 0;
};
}//namespace bk_conq

#endif /* BK_CONQ_UNBOUNDEDQUEUE_HPP */

