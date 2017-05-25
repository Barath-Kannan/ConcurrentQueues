/*
 * File:   bounded_queue.hpp
 * Author: Barath Kannan
 * Bounded queue tag
 * Created on 17 October 2016, 1:34 PM
 */

#ifndef BK_CONQ_BOUNDEDQUEUE_HPP
#define BK_CONQ_BOUNDEDQUEUE_HPP

namespace bk_conq {

class bounded_queue_tag {};

template <typename T>
class bounded_queue_typed_tag {};

template <typename T, typename BASE>
class bounded_queue : public bounded_queue_typed_tag<T>, public bounded_queue_tag {
public:
    bool sp_enqueue(T&& input) {
        return base()->sp_enqueue_impl(std::move(input));
    }

    bool sp_enqueue(const T& input) {
        return base()->sp_enqueue_impl(input);
    }

    bool mp_enqueue(T&& input) {
        return base()->mp_enqueue_impl(std::move(input));
    }

    bool mp_enqueue(const T& input) {
        return base()->mp_enqueue_impl(input);
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
    inline BASE* base() {
        return static_cast<BASE*>(this);
    }
};

}//namespace bk_conq

#endif /* BK_CONQ_BOUNDEDQUEUE_HPP */

