/*
 * File:   bounded_queue.hpp
 * Author: Barath Kannan
 * Abstract specification for a bounded producer consumer queue
 * Created on 17 October 2016, 1:34 PM
 */

#ifndef BK_CONQ_BOUNDEDQUEUE_HPP
#define BK_CONQ_BOUNDEDQUEUE_HPP

namespace bk_conq {
template<typename T>
class bounded_queue {
public:
	virtual ~bounded_queue() {};

	virtual bool sp_enqueue(T&& input) = 0;
	virtual bool sp_enqueue(const T& input) = 0;
	virtual bool mp_enqueue(T&& input) = 0;
	virtual bool mp_enqueue(const T& input) = 0;

	virtual bool sc_dequeue(T& output) = 0;
	virtual bool mc_dequeue(T& output) = 0;
};
}//namespace bk_conq

#endif /* BK_CONQ_BOUNDEDQUEUE_HPP */

