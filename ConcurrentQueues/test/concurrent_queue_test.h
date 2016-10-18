/* 
 * File:   concurrent_queue_test.h
 * Author: Barath Kannan
 *
 * Created on 27 August 2016, 9:28 PM
 */

#ifndef CONCURRENT_QUEUE_TEST_H
#define CONCURRENT_QUEUE_TEST_H

#include <gtest/gtest.h>
#include <bk_conq/array_queue.hpp>
#include <bk_conq/vector_queue.hpp>
#include <bk_conq/list_queue.hpp>
#include <bk_conq/cache_queue.hpp>
#include <bk_conq/multilist_array_queue.hpp>
#include <bk_conq/multilist_vector_queue.hpp>
#include "basic_timer.h"

struct TestParameters {
    uint32_t nReaders;
    uint32_t nWriters;
    uint32_t nElements;
};


class QueueTest : public testing::Test,
public testing::WithParamInterface< ::testing::tuple<uint32_t, uint32_t, uint32_t> > {
public:
    virtual void SetUp();
    virtual void TearDown();

protected:
    std::vector<basic_timer> readers;
    std::vector<basic_timer> writers;
    TestParameters _params;

    template<typename T, typename R, typename ...Args>
    void GenericTest(TestParameters params, std::function<void(T&, R&) > dequeueOperation, std::function<void(T&, R) > enqueueOperation, Args... args) {
        static T q{args...};
        std::vector<std::thread> l;
        for (size_t i = 0; i < params.nReaders; ++i) {
            l.emplace_back([&, i]() {
                readers[i].start();
                uint64_t res;
                for (size_t j = 0; j < _params.nElements / _params.nReaders; ++j) {
                    dequeueOperation(q, res);
                }
                if (i == 0) {
                    size_t remainder = _params.nElements - ((_params.nElements / _params.nReaders) * _params.nReaders);
                    for (size_t j = 0; j < remainder; ++j) {
                        dequeueOperation(q, res);
                    }
                }
                readers[i].stop();
            });
        }
        for (size_t i = 0; i < _params.nWriters; ++i) {
            l.emplace_back([&, i]() {
                writers[i].start();
                for (size_t j = 0; j < _params.nElements / _params.nWriters; ++j) {
                    enqueueOperation(q, j);
                }
                if (i == 0) {
                    size_t remainder = _params.nElements - ((_params.nElements / _params.nWriters) * _params.nWriters);
                    for (size_t j = 0; j < remainder; ++j) {
                        enqueueOperation(q, j);
                    }
                }
                writers[i].stop();
            });
        }
        for (size_t i = 0; i < _params.nReaders + _params.nWriters; ++i) {
            l[i].join();
        }
    }
    
    template<typename T, typename R, typename... Args>
    typename std::enable_if_t<std::is_base_of<bk_conq::unbounded_queue<R>, T>::value>
    BusyTest(TestParameters params, Args&&... args){
        std::function<void(T&, R & item) > dfunc = ([](T& q, R & item) {
            while (!q.mc_dequeue(item));
        });
        std::function<void(T&, R item) > efunc = ([](T& q, R item) {
            q.mp_enqueue(item);
        });
        GenericTest(params, dfunc, efunc, args...);
    }

    template<typename T, typename R, typename ...Args>
    typename std::enable_if_t<std::is_base_of<bk_conq::bounded_queue<R>, T>::value>
    BusyTest(TestParameters params, Args&&... args){
        std::function<void(T&, R & item) > dfunc = ([](T& q, R & item) {
            while (!q.mc_dequeue(item));
        });
        std::function<void(T&, R item) > efunc = ([](T& q, R item) {
            while (!q.mp_enqueue(item));
        });
        GenericTest(params, dfunc, efunc, args...);
    }

    template<typename T, typename R, typename... Args>
    typename std::enable_if_t<std::is_base_of<bk_conq::unbounded_queue<R>, T>::value>
    SleepTest(TestParameters params, Args&&... args){
        std::function<void(T&, R & item) > dfunc = ([](T& q, R & item) {
            while (!q.mc_dequeue(item)){
                std::this_thread::sleep_for(std::chrono::nanoseconds(10));
            }
        });
        std::function<void(T&, R item) > efunc = ([](T& q, R item) {
            q.mp_enqueue(item);
        });
        GenericTest(params, dfunc, efunc, args...);
    }

    template<typename T, typename R, typename ...Args>
    typename std::enable_if_t<std::is_base_of<bk_conq::bounded_queue<R>, T>::value>
    SleepTest(TestParameters params, Args&&... args){
        std::function<void(T&, R & item) > dfunc = ([](T& q, R & item) {
            while (!q.mc_dequeue(item)){
                std::this_thread::sleep_for(std::chrono::nanoseconds(10));
            }
        });
        std::function<void(T&, R item) > efunc = ([](T& q, R item) {
            while (!q.mp_enqueue(item));
        });
        GenericTest(params, dfunc, efunc, args...);
    }
    
    template<typename T, typename R, typename... Args>
    typename std::enable_if_t<std::is_base_of<bk_conq::unbounded_queue<R>, T>::value>
    BackoffTest(TestParameters params, Args&&... args){
        std::function<void(T&, R & item) > dfunc = ([](T& q, R & item) {
            auto wait_time = std::chrono::nanoseconds(1);
            while (!q.mc_dequeue(item)){
                std::this_thread::sleep_for(wait_time);
                wait_time *= 2;
            }
        });
        std::function<void(T&, R item) > efunc = ([](T& q, R item) {
            q.mp_enqueue(item);
        });
        GenericTest(params, dfunc, efunc, args...);
    }

    template<typename T, typename R, typename ...Args>
    typename std::enable_if_t<std::is_base_of<bk_conq::bounded_queue<R>, T>::value>
    BackoffTest(TestParameters params, Args&&... args){
        std::function<void(T&, R & item) > dfunc = ([](T& q, R & item) {
            auto wait_time = std::chrono::nanoseconds(1);
            while (!q.mc_dequeue(item)){
                std::this_thread::sleep_for(wait_time);
                wait_time *= 2;
            }
        });
        std::function<void(T&, R item) > efunc = ([](T& q, R item) {
            while (!q.mp_enqueue(item));
        });
        GenericTest(params, dfunc, efunc, args...);
    }
};
#endif /* CONCURRENT_QUEUE_TEST_H */

