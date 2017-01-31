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
#include "basic_timer.h"

enum QueueTestType : uint32_t{
	BUSY_TEST = 0,
	YIELD_TEST = 1,
	SLEEP_TEST = 2,
	BACKOFF_TEST = 3
};

struct TestParameters {
    uint32_t nReaders;
    uint32_t nWriters;
    uint32_t nElements;
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
public testing::WithParamInterface< ::testing::tuple<uint32_t, uint32_t, uint32_t, size_t, size_t, QueueTestType> > {
public:
	typedef size_t queue_test_type_t;

    virtual void SetUp();
    virtual void TearDown();
protected:
    std::vector<basic_timer> readers;
    std::vector<basic_timer> writers;
    TestParameters _params;
	std::thread _monitorThread;
	std::atomic<bool> _doneFlag;

    template<typename T, typename R, typename ...Args>
    void GenericTest(std::function<void(T&, R&) > dequeueOperation, std::function<void(T&, R) > enqueueOperation, Args... args) {
		T q{ args... };
        std::vector<std::thread> l;
        for (size_t i = 0; i < _params.nReaders; ++i) {
            l.emplace_back([&, i]() {
                readers[i].start();
				queue_test_type_t res;
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

	template<typename T, typename R>
	std::function<void(T&,R&)> generateDequeueFunctionNonblocking() {
		return ([testType = _params.testType](T& q, R& item) {
			auto wait_time = std::chrono::nanoseconds(1);
			switch (testType) {
			case BUSY_TEST: while (!q.mc_dequeue(item)); break;
			case YIELD_TEST: while (!q.mc_dequeue(item)) { std::this_thread::yield(); } break;
			case SLEEP_TEST: while (!q.mc_dequeue(item)) { std::this_thread::sleep_for(std::chrono::nanoseconds(10)); } break;
			case BACKOFF_TEST: while (!q.mc_dequeue(item)) { std::this_thread::sleep_for(wait_time); wait_time *= 2; } break;
			default: break;
			}
		});
	}

	template<typename T, typename R>
	std::function<void(T&,R&)> generateDequeueFunctionBlocking() {
		return ([](T& q, R& item) {
			q.mc_dequeue(item);
		});
	}

	template<typename T, typename R>
	std::function<void(T&,R)> generateEnqueueFunctionNonblocking() {
		return ([testType = _params.testType](T& q, R item) {
			auto wait_time = std::chrono::nanoseconds(1);
			switch (testType) {
			case BUSY_TEST: while (!q.mp_enqueue(item)); break;
			case YIELD_TEST: while (!q.mp_enqueue(item)) { std::this_thread::yield(); } break;
			case SLEEP_TEST: while (!q.mp_enqueue(item)) { std::this_thread::sleep_for(std::chrono::nanoseconds(10)); } break;
			case BACKOFF_TEST: while (!q.mp_enqueue(item)) { std::this_thread::sleep_for(wait_time); wait_time *= 2; } break;
			default: break;
			}
		});
	}

	template<typename T, typename R>
	std::function<void(T&,R)> generateEnqueueFunctionBlocking() {
		return ([](T& q, R item) {
			q.mp_enqueue(item);
		});
	}

	template<typename T, typename R, typename... Args>
	typename std::enable_if_t<std::is_base_of<bk_conq::unbounded_queue, T>::value>
	TemplatedTest(Args&&... args) {
		auto dequeueFunction = generateDequeueFunctionNonblocking<T, R>();
		auto enqueueFunction = generateEnqueueFunctionBlocking<T, R>();
		GenericTest(dequeueFunction, enqueueFunction, args...);
	}

	template<typename T, typename R, typename ...Args>
	typename std::enable_if_t<std::is_base_of<bk_conq::bounded_queue, T>::value>
	TemplatedTest(Args&&... args) {
		auto dequeueFunction = generateDequeueFunctionNonblocking<T, R>();
		auto enqueueFunction = generateEnqueueFunctionNonblocking<T, R>();
		GenericTest(dequeueFunction, enqueueFunction, _params.queueSize, args...);
	}

	template <typename T, typename R, typename... Args>
	typename std::enable_if_t<std::is_base_of<bk_conq::unbounded_queue, T>::value>
	BlockingTest(Args&&... args) {
		auto dequeueFunction = generateDequeueFunctionBlocking<T, R>();
		auto enqueueFunction = generateEnqueueFunctionBlocking<T, R>();
		GenericTest(dequeueFunction, enqueueFunction, args...);
	}

	template <typename T, typename R, typename... Args>
	typename std::enable_if_t<std::is_base_of<bk_conq::bounded_queue, T>::value>
	BlockingTest(Args&&... args) {
		auto dequeueFunction = generateDequeueFunctionBlocking<T, R>();
		auto enqueueFunction = generateEnqueueFunctionBlocking<T, R>();
		GenericTest(dequeueFunction, enqueueFunction, _params.queueSize, args...);
	}

};
#endif /* CONCURRENT_QUEUE_TEST_H */
