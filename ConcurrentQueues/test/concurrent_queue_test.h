/*
 * File:   concurrent_queue_test.h
 * Author: Barath Kannan
 *
 * Created on 27 August 2016, 9:28 PM
 */

#ifndef CONCURRENT_QUEUE_TEST_H
#define CONCURRENT_QUEUE_TEST_H

#include <gtest/gtest.h>
#include <iostream>
#include <bk_conq/blocking_unbounded_queue.hpp>
#include <bk_conq/blocking_bounded_queue.hpp>
#include <bk_conq/multi_bounded_queue.hpp>
#include <bk_conq/multi_unbounded_queue.hpp>
#include <bk_conq/bounded_list_queue.hpp>
#include <bk_conq/vector_queue.hpp>
#include <bk_conq/list_queue.hpp>
#include <bk_conq/chain_queue.hpp>
#include "basic_timer.h"

enum QueueTestType : uint32_t {
    BUSY_TEST = 0,
    YIELD_TEST = 1,
    SLEEP_TEST = 2,
    BACKOFF_TEST = 3
};

struct TestParameters {
    size_t nReaders;
    size_t nWriters;
    size_t nElements;
    size_t queueSize;
    size_t subqueueSize;
    QueueTestType testType;
};

struct BigThing {
    size_t value;
    char padding[256];
    void operator=(const size_t& v) {
        value = v;
    }
    BigThing(const size_t& v) {
        value = v;
    }
    BigThing() {}
};


class QueueTest : public testing::Test,
    public testing::WithParamInterface< ::testing::tuple<size_t, size_t, size_t, size_t, size_t, QueueTestType> > {
public:
    typedef size_t queue_test_type_t;

    virtual void SetUp();
    virtual void TearDown();
protected:
    std::vector<basic_timer> readers;
    std::vector<basic_timer> writers;
    TestParameters _params;
    std::thread _monitorThread;
    std::atomic<bool> _startFlag{ false };
    std::atomic<size_t> _sync{ 0 };

    template<typename T, typename R, typename ...Args>
    void GenericTest(std::function<void(T&, R&) > dequeueOperation, std::function<void(T&, R) > enqueueOperation, bool prefill, Args... args) {
        T q{ args... };
        std::vector<std::thread> l;
        for (int i = 0; i < (prefill ? 2 : 1); ++i) {
            _startFlag.store(false);
            _sync.store(0);
            l.clear();
            for (size_t i = 0; i < _params.nReaders; ++i) {
                l.emplace_back([&, i]() {
                    ++_sync;
                    while (!_startFlag.load(std::memory_order_acquire)) { std::this_thread::yield(); };
                    readers[i].start();
                    queue_test_type_t res;
                    for (size_t j = 0; j < _params.nElements / _params.nReaders; ++j) {
                        dequeueOperation(q, res);
                        //if (j % 100000 == 0) std::cout << "d" << j << std::endl;
                        //if (_params.nElements/_params.nReaders -j < 100000) std::cout << "z" << j << std::endl;
                        //if (_params.nElements / _params.nReaders - j < 100000) break;
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
                    ++_sync;
                    while (!_startFlag.load(std::memory_order_acquire)) { std::this_thread::yield(); };
                    writers[i].start();
                    for (size_t j = 0; j < _params.nElements / _params.nWriters; ++j) {
                        enqueueOperation(q, j);
                        //if (j % 100000 == 0) std::cout << "e" << j << std::endl;
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
            while (_sync.load() != _params.nWriters + _params.nReaders) { std::this_thread::yield(); };
            _startFlag.store(true, std::memory_order_release);
            for (size_t i = 0; i < _params.nReaders + _params.nWriters; ++i) {
                l[i].join();
            }
        }

    }

    auto generateBusyDequeue() {
        return ([](auto& q, auto& item) {
            while (!q.mc_dequeue(item));
        });
    }

    auto generateYieldDequeue() {
        return ([](auto& q, auto& item) {
            while (!q.mc_dequeue(item)) { std::this_thread::yield(); }
        });
    }

    auto generateSleepDequeue() {
        return ([](auto& q, auto& item) {
            while (!q.mc_dequeue(item)) { std::this_thread::sleep_for(std::chrono::nanoseconds(10)); }
        });
    }

    auto generateBackoffDequeue() {
        return ([](auto& q, auto& item) {
            auto wait_time = std::chrono::nanoseconds(1);
            while (!q.mc_dequeue(item)) {
                std::this_thread::sleep_for(wait_time);
                wait_time *= 2;
            }
        });
    }

    auto generateBusyEnqueue() {
        return ([](auto& q, auto item) {
            while (!q.mp_enqueue(item));
        });
    }

    auto generateYieldEnqueue() {
        return ([](auto& q, auto item) {
            while (!q.mp_enqueue(item)) { std::this_thread::yield(); }
        });
    }

    auto generateSleepEnqueue() {
        return ([](auto& q, auto item) {
            while (!q.mp_enqueue(item)) { std::this_thread::sleep_for(std::chrono::nanoseconds(10)); }
        });
    }

    auto generateBackoffEnqueue() {
        return ([](auto& q, auto item) {
            auto wait_time = std::chrono::nanoseconds(1);
            while (!q.mp_enqueue(item)) { std::this_thread::sleep_for(wait_time); wait_time *= 2; }
        });
    }

    template<typename T, typename R>
    std::function<void(T&, R&)> generateDequeueFunctionNonblocking() {
        switch (_params.testType) {
        case BUSY_TEST: return generateBusyDequeue();
        case YIELD_TEST: return generateYieldDequeue();
        case SLEEP_TEST: return generateSleepDequeue();
        case BACKOFF_TEST: return generateBackoffDequeue();
        default: return generateBusyDequeue();
        }
    }

    template<typename T, typename R>
    std::function<void(T&, R&)> generateDequeueFunctionBlocking() {
        return ([](T& q, R& item) {
            q.mc_dequeue(item);
        });
    }

    template<typename T, typename R>
    std::function<void(T&, R)> generateEnqueueFunctionNonblocking() {
        switch (_params.testType) {
        case BUSY_TEST: return generateBusyEnqueue();
        case YIELD_TEST: return generateYieldEnqueue();
        case SLEEP_TEST: return generateSleepEnqueue();
        case BACKOFF_TEST: return generateBackoffEnqueue();
        default: return generateBusyEnqueue();
        }
    }

    template<typename T, typename R>
    std::function<void(T&, R)> generateEnqueueFunctionBlocking() {
        return ([](T& q, R item) {
            q.mp_enqueue(item);
        });
    }

    template<typename T, typename R, typename... Args>
    typename std::enable_if_t<std::is_base_of<bk_conq::unbounded_queue_typed_tag<R>, T>::value>
        TemplatedTest(bool prefill, Args&&... args) {
        auto dequeueFunction = generateDequeueFunctionNonblocking<T, R>();
        auto enqueueFunction = generateEnqueueFunctionBlocking<T, R>();
        GenericTest(dequeueFunction, enqueueFunction, prefill, args...);
    }

    template<typename T, typename R, typename ...Args>
    typename std::enable_if_t<std::is_base_of<bk_conq::bounded_queue_typed_tag<R>, T>::value>
        TemplatedTest(Args&&... args) {
        auto dequeueFunction = generateDequeueFunctionNonblocking<T, R>();
        auto enqueueFunction = generateEnqueueFunctionNonblocking<T, R>();
        GenericTest(dequeueFunction, enqueueFunction, false, _params.queueSize, args...);
    }

    template<typename T, typename R, typename ...Args>
    typename std::enable_if_t<std::is_base_of<bk_conq::bounded_queue_typed_tag<R>, T>::value>
        TemplatedBoundedTest(Args&&... args) {
        auto dequeueFunction = generateDequeueFunctionNonblocking<T, R>();
        auto enqueueFunction = generateEnqueueFunctionNonblocking<T, R>();
        GenericTest(dequeueFunction, enqueueFunction, false, args...);
    }


    template <typename T, typename R, typename... Args>
    typename std::enable_if_t<std::is_base_of<bk_conq::unbounded_queue_typed_tag<R>, T>::value>
        BlockingTest(bool prefill, Args&&... args) {
        auto dequeueFunction = generateDequeueFunctionBlocking<T, R>();
        auto enqueueFunction = generateEnqueueFunctionBlocking<T, R>();
        GenericTest(dequeueFunction, enqueueFunction, prefill, args...);
    }

    template <typename T, typename R, typename... Args>
    typename std::enable_if_t<std::is_base_of<bk_conq::bounded_queue_typed_tag<R>, T>::value>
        BlockingTest(Args&&... args) {
        auto dequeueFunction = generateDequeueFunctionBlocking<T, R>();
        auto enqueueFunction = generateEnqueueFunctionBlocking<T, R>();
        GenericTest(dequeueFunction, enqueueFunction, false, _params.queueSize, args...);
    }

};
#endif /* CONCURRENT_QUEUE_TEST_H */
