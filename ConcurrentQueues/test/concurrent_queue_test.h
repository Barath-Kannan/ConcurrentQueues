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
	SLEEP_TEST = 1,
	BACKOFF_TEST = 2
};

struct TestParameters {
    uint32_t nReaders;
    uint32_t nWriters;
    uint32_t nElements;
	QueueTestType testType;
};

namespace {
	//const size_t BOUNDED_QUEUE_SIZE = 2'097'152u;
	const size_t BOUNDED_QUEUE_SIZE = 131'072u;
	const size_t SUBQUEUE_SIZE = 32u;
	const size_t PADDING_SIZE = 256u;
	//const size_t PADDING_SIZE = 8u;
}
struct BigThing {
	size_t value;
	char padding[PADDING_SIZE];
	void operator=(const size_t& v) {
		value = v;
	}
	BigThing(const size_t& v) {
		value = v;
	}
	BigThing() {}
};

class QueueTest : public testing::Test,
public testing::WithParamInterface< ::testing::tuple<uint32_t, uint32_t, uint32_t, QueueTestType> > {
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
    void GenericTest(TestParameters params, std::function<void(T&, R&) > dequeueOperation, std::function<void(T&, R) > enqueueOperation, Args... args) {
		static T q{ args... };
        std::vector<std::thread> l;
        for (size_t i = 0; i < params.nReaders; ++i) {
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

	template<typename T, typename R, typename... Args>
	typename std::enable_if_t<std::is_base_of<bk_conq::unbounded_queue, T>::value>
	TemplatedTest(TestParameters params, Args&&... args) {
		std::function<void(T&, R & item) > dfunc = ([testType = params.testType](T& q, R & item) {
			switch (testType) {
				case BUSY_TEST: while (!q.mc_dequeue(item)); break;
				case SLEEP_TEST: while (!q.mc_dequeue(item)) { std::this_thread::sleep_for(std::chrono::nanoseconds(10)); } break;
				case BACKOFF_TEST: auto wait_time = std::chrono::nanoseconds(1); while (!q.mc_dequeue(item)) { std::this_thread::sleep_for(wait_time); wait_time *= 2; } break;
				default: break;
			}
		});
		std::function<void(T&, R item) > efunc = ([](T& q, R item) {
			q.mp_enqueue(item);
			
		});
		GenericTest(params, dfunc, efunc, args...);
	}


	template<typename T, typename R, typename ...Args>
	typename std::enable_if_t<std::is_base_of<bk_conq::bounded_queue, T>::value>
	TemplatedTest(TestParameters params, Args&&... args) {
		std::function<void(T&, R & item) > dfunc = ([testType = params.testType](T& q, R & item) {
			switch (testType) {
				case BUSY_TEST:  while (!q.mc_dequeue(item)); break;
				case SLEEP_TEST: while (!q.mc_dequeue(item)) { std::this_thread::sleep_for(std::chrono::nanoseconds(10)); } break;
				case BACKOFF_TEST: auto wait_time = std::chrono::nanoseconds(1); while (!q.mc_dequeue(item)) { std::this_thread::sleep_for(wait_time); wait_time *= 2; } break;
				default: break;
			}
			
		});
		std::function<void(T&, R item) > efunc = ([testType = params.testType](T& q, R item) {
			switch (testType) {
				case(BUSY_TEST):  while (!q.mp_enqueue(item)); break;
				case(SLEEP_TEST): while (!q.mp_enqueue(item)) { std::this_thread::sleep_for(std::chrono::nanoseconds(10)); } break;
				case(BACKOFF_TEST): auto wait_time = std::chrono::nanoseconds(1); while (!q.mp_enqueue(item)) { std::this_thread::sleep_for(wait_time); wait_time *= 2; } break;
				default: break;
			}
		});
		GenericTest(params, dfunc, efunc, args...);
	}


	template <typename T, typename R, typename... Args>
	void BlockingTest(TestParameters params, Args&&... args) {
		std::function<void(T&, R & item) > dfunc = ([](T& q, R & item) {
			q.mc_dequeue(item);
		});
		std::function<void(T&, R item) > efunc = ([](T& q, R item) {
			q.mp_enqueue(item);
		});
		GenericTest(params, dfunc, efunc, args...);
	}

};
#endif /* CONCURRENT_QUEUE_TEST_H */
