#include "concurrent_queue_test.h"
#include <iostream>
#include <chrono>
#include <type_traits>
#include <queue>

using ::testing::Values;
using ::testing::Combine;
using ::testing::Types;
using std::cout;
using std::endl;

std::ostream& operator<<(std::ostream& os, const std::chrono::duration<double>& t) {
    double val;
    if ((val = std::chrono::duration<double>(t).count()) > 1.0){
        os << val << " seconds";
    }
    else if ((val = std::chrono::duration<double, std::milli>(t).count()) > 1.0){
        os << val << " milliseconds";
    }
    else if ((val = std::chrono::duration<double, std::micro>(t).count()) > 1.0){
        os << val << " microseconds";
    }
    else{
        val = std::chrono::duration<double, std::nano>(t).count();
        os << val << " nanoseconds";
    }
    return os;
}

void QueueTest::SetUp(){
    auto tupleParams = GetParam();
    _params = TestParameters{::testing::get<0>(tupleParams), ::testing::get<1>(tupleParams), ::testing::get<2>(tupleParams), ::testing::get<3>(tupleParams)};
    readers.resize(_params.nReaders, basic_timer());
    writers.resize(_params.nWriters, basic_timer());
    cout << "Readers: " << _params.nReaders << endl;
    cout << "Writers: " << _params.nWriters << endl;
    cout << "Elements: " << _params.nElements << endl;
	cout << "Test Type: ";
	switch (_params.testType) {
		case BUSY_TEST: cout << "Busy Test"; break;
		case SLEEP_TEST: cout << "Sleep Test"; break;
		case BACKOFF_TEST: cout << "Backoff Test"; break;
		default: break;
	}
	cout << endl;
}

void QueueTest::TearDown(){
    cout << "Enqueue:" << endl;
    auto writeDur = writers[0].getElapsedDuration();
    auto writeMax = writers[0].getElapsedDuration();
    int toMeasure = 1;
    for (size_t i=1; i<writers.size(); ++i){
        auto dur = writers[i].getElapsedDuration();
        if (dur > std::chrono::nanoseconds(1)){
            writeDur += dur;
            if (dur > writeMax) writeMax = dur;
            ++toMeasure;
        }
    }
    writeDur/=toMeasure;
    cout << "Max write thread duration: " << writeMax << endl;
    cout << "Average write thread duration: " << writeDur << endl;
    writeDur/=_params.nElements;
    cout << "Average time per enqueue: " << writeDur << endl;
    
    cout << "Dequeue:" << endl;
    auto readDur = readers[0].getElapsedDuration();
    auto readMax = readers[0].getElapsedDuration();
    toMeasure = 1;
    for (size_t i=1; i<readers.size(); ++i){
        auto dur = readers[i].getElapsedDuration();
        if (dur > std::chrono::nanoseconds(1)){
            readDur += dur;
            if (dur > readMax) readMax = dur;
            ++toMeasure;
        }        
    }
    readDur/=toMeasure;
    cout << "Max read thread duration: " << readMax << endl;
    cout << "Average read thread duration: " << readDur << endl;
    readDur/=_params.nElements;
    cout << "Average time per dequeue: " << readDur << endl;
}

namespace ListTests{
	using qtype = bk_conq::list_queue<QueueTest::queue_test_type_t>;
	using mqtype = bk_conq::multi_unbounded_queue<qtype>;
	using bqtype = bk_conq::blocking_unbounded_queue<qtype>;
	using bmqtype = bk_conq::blocking_unbounded_queue<mqtype>;

	TEST_P(QueueTest, list_queue) {
		QueueTest::TemplatedTest<qtype, queue_test_type_t>(_params);
	}

	TEST_P(QueueTest, list_queue_blocking) {
		QueueTest::BlockingTest<bqtype, queue_test_type_t>(_params);
	}

	TEST_P(QueueTest, multi_list_queue) {
		QueueTest::TemplatedTest<mqtype, queue_test_type_t>(_params, SUBQUEUE_SIZE);
	}

	TEST_P(QueueTest, multi_list_queue_blocking) {
		QueueTest::BlockingTest<bmqtype, queue_test_type_t>(_params, SUBQUEUE_SIZE);
	}
}

namespace BoundedListQueue {
	using qtype = bk_conq::bounded_list_queue<QueueTest::queue_test_type_t>;
	using mqtype = bk_conq::multi_bounded_queue<qtype>;
	using bqtype = bk_conq::blocking_bounded_queue<qtype>;
	using bmqtype = bk_conq::blocking_bounded_queue<mqtype>;

	TEST_P(QueueTest, bounded_list_queue) {
		QueueTest::TemplatedTest<qtype, queue_test_type_t>(_params, BOUNDED_QUEUE_SIZE);
	}

	TEST_P(QueueTest, bounded_list_queue_blocking) {
		QueueTest::BlockingTest<bqtype, queue_test_type_t>(_params, BOUNDED_QUEUE_SIZE);
	}

	TEST_P(QueueTest, multi_bounded_list_queue) {
		QueueTest::TemplatedTest<mqtype, queue_test_type_t>(_params, BOUNDED_QUEUE_SIZE, SUBQUEUE_SIZE);
	}

	TEST_P(QueueTest, multi_bounded_list_queue_blocking) {
		QueueTest::BlockingTest<bmqtype, queue_test_type_t>(_params, BOUNDED_QUEUE_SIZE, SUBQUEUE_SIZE);
	}
}

namespace VectorQueue {
	using qtype = bk_conq::vector_queue<QueueTest::queue_test_type_t>;
	using mqtype = bk_conq::multi_bounded_queue<qtype>;
	using bqtype = bk_conq::blocking_bounded_queue<qtype>;
	using bmqtype = bk_conq::blocking_bounded_queue<mqtype>;

	TEST_P(QueueTest, vector_queue) {
		QueueTest::TemplatedTest<qtype, queue_test_type_t>(_params, BOUNDED_QUEUE_SIZE);
	}

	TEST_P(QueueTest, vector_queue_blocking) {
		QueueTest::BlockingTest<bqtype, queue_test_type_t>(_params, BOUNDED_QUEUE_SIZE);
	}

	TEST_P(QueueTest, multi_vector_queue) {
		QueueTest::TemplatedTest<mqtype, queue_test_type_t>(_params, BOUNDED_QUEUE_SIZE, SUBQUEUE_SIZE);
	}

	TEST_P(QueueTest, multi_vector_queue_blocking) {
		QueueTest::BlockingTest<bmqtype, queue_test_type_t>(_params, BOUNDED_QUEUE_SIZE, SUBQUEUE_SIZE);
	}
}

INSTANTIATE_TEST_CASE_P(
        queue_benchmark,
        QueueTest,
        testing::Combine(
        Values(1, 2, 4, 8, 16, 32, 256), //readers
        Values(1, 2, 4, 8, 16, 32, 256), //writers
        Values(1e6, 1e7, 1e8, 1e9), //elements
		Values(QueueTestType::BUSY_TEST, QueueTestType::SLEEP_TEST, QueueTestType::BACKOFF_TEST)) //test type
        );