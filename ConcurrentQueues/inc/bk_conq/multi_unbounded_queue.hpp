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
#include <numeric>
#include <bk_conq/unbounded_queue.hpp>
#include <bk_conq/details/tlos.hpp>

namespace bk_conq {

template<typename TT>
class multi_unbounded_queue;

template <template <typename> class Q, typename T>
class multi_unbounded_queue<Q<T>> : public unbounded_queue<T, multi_unbounded_queue<Q<T>>> {
    friend unbounded_queue<T, multi_unbounded_queue<Q<T>>>;
public:
    multi_unbounded_queue(size_t subqueues) :
        _q(subqueues),
        _hitlist([&]() {return hitlist_sequence(); }),
        _enqueue_identifier([&]() { return get_enqueue_index(); }, [&](padded_unbounded_queue* indx) {return return_enqueue_index(indx); })
    {
        static_assert(std::is_base_of<bk_conq::unbounded_queue_typed_tag<T>, Q<T>>::value, "Q<T> must be an unbounded queue");
    }

    multi_unbounded_queue(const multi_unbounded_queue&) = delete;
    void operator=(const multi_unbounded_queue&) = delete;

protected:
    template <typename R>
    void sp_enqueue_impl(R&& input) {
        _enqueue_identifier.get()->sp_enqueue(std::forward<R>(input));
    }

    template <typename R>
    void mp_enqueue_impl(R&& input) {
        _enqueue_identifier.get()->mp_enqueue(std::forward<R>(input));
    }

    bool sc_dequeue_impl(T& output) {
        auto& hitlist = _hitlist.get();
        for (auto it = hitlist.cbegin(); it != hitlist.cend(); ++it) {
            if (_q[*it].sc_dequeue(output)) {
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
            if (_q[*it].mc_dequeue_uncontended(output)) {
                if (hitlist.cbegin() == it) return true;
                //funky magic - range erase returns an iterator, but an empty range is provided so contents aren't changed
                //this converts a const iterator to an iterator in constant time
                auto nonconstit = hitlist.erase(it, it);
                for (auto it2 = hitlist.begin(); it2 != nonconstit; ++it2) std::iter_swap(nonconstit, it2);
                return true;
            }
        }
        for (auto it = hitlist.cbegin(); it != hitlist.cend(); ++it) {
            if (_q[*it].mc_dequeue(output)) {
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
            if (_q[*it].mc_dequeue_uncontended(output)) {
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
    class padded_unbounded_queue : public Q<T> {
        char padding[64];
    };

    std::vector<size_t> hitlist_sequence() {
        std::vector<size_t> hitlist(_q.size());
        std::iota(hitlist.begin(), hitlist.end(), 0);
        return hitlist;
    }

    padded_unbounded_queue* get_enqueue_index() {
        std::lock_guard<std::mutex> lock(_m);
        if (_unused_enqueue_indexes.empty()) {
            return &_q[(_enqueue_index++) % _q.size()];
        }
        size_t ret = _unused_enqueue_indexes.back();
        _unused_enqueue_indexes.pop_back();
        return &_q[ret];
    }

    void return_enqueue_index(padded_unbounded_queue* index) {
        std::lock_guard<std::mutex> lock(_m);
        for (size_t i = 0; i < _q.size(); ++i) {
            if (&_q[i] == index) {
                _unused_enqueue_indexes.push_back(i);
                return;
            }
        }
    }

    std::vector<padded_unbounded_queue> _q;
    std::vector<size_t> _unused_enqueue_indexes;
    size_t				_enqueue_index{ 0 };
    std::mutex			_m;

    details::tlos<std::vector<size_t>, multi_unbounded_queue<Q<T>>> _hitlist;
    details::tlos<padded_unbounded_queue*, multi_unbounded_queue<Q<T>>> _enqueue_identifier;
};

}//namespace bk_conq

#endif /* BK_CONQ_MULTI_UNBOUNDED_QUEUE_HPP */
