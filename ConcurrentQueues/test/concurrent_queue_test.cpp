#include "concurrent_queue_test.h"
#include <iostream>
#include <chrono>
#include <type_traits>

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
    _params = TestParameters{::testing::get<0>(tupleParams), ::testing::get<1>(tupleParams), ::testing::get<2>(tupleParams)};
    readers.resize(_params.nReaders, basic_timer());
    writers.resize(_params.nWriters, basic_timer());
    cout << "Readers: " << _params.nReaders << endl;
    cout << "Writers: " << _params.nWriters << endl;
    cout << "Elements: " << _params.nElements << endl;
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

TEST_P(QueueTest, list_queue_busy){
    QueueTest::BusyTest<bk_conq::list_queue<queue_test_type_t>, queue_test_type_t>(_params);
}

TEST_P(QueueTest, array_queue_busy){
    QueueTest::BusyTest<bk_conq::array_queue<queue_test_type_t, BOUNDED_QUEUE_SIZE>, queue_test_type_t>(_params);
}

TEST_P(QueueTest, vector_queue_busy){
    QueueTest::BusyTest<bk_conq::vector_queue<queue_test_type_t>, queue_test_type_t>(_params, BOUNDED_QUEUE_SIZE);
}

TEST_P(QueueTest, cache_queue_busy){
    QueueTest::BusyTest<bk_conq::cache_queue<queue_test_type_t, BOUNDED_QUEUE_SIZE>, queue_test_type_t>(_params);
}

TEST_P(QueueTest, multilist_array_queue_busy){
    QueueTest::BusyTest<bk_conq::multilist_array_queue<queue_test_type_t, SUBQUEUE_SIZE>, queue_test_type_t>(_params);
}

TEST_P(QueueTest, multilist_vector_queue_busy){
    QueueTest::BusyTest<bk_conq::multilist_vector_queue<queue_test_type_t>, queue_test_type_t>(_params, SUBQUEUE_SIZE);
}

TEST_P(QueueTest, list_queue_sleep){
    QueueTest::SleepTest<bk_conq::list_queue<queue_test_type_t>, queue_test_type_t>(_params);
}

TEST_P(QueueTest, array_queue_sleep){
    QueueTest::SleepTest<bk_conq::array_queue<queue_test_type_t, BOUNDED_QUEUE_SIZE>, queue_test_type_t>(_params);
}

TEST_P(QueueTest, vector_queue_sleep){
    QueueTest::SleepTest<bk_conq::vector_queue<queue_test_type_t>, queue_test_type_t>(_params, BOUNDED_QUEUE_SIZE);
}

TEST_P(QueueTest, cache_queue_sleep){
    QueueTest::SleepTest<bk_conq::cache_queue<queue_test_type_t, BOUNDED_QUEUE_SIZE>, queue_test_type_t>(_params);
}

TEST_P(QueueTest, multilist_array_queue_sleep){
    QueueTest::SleepTest<bk_conq::multilist_array_queue<queue_test_type_t, SUBQUEUE_SIZE>, queue_test_type_t>(_params);
}

TEST_P(QueueTest, multilist_vector_queue_sleep){
    QueueTest::SleepTest<bk_conq::multilist_vector_queue<queue_test_type_t>, queue_test_type_t>(_params, SUBQUEUE_SIZE);
}

TEST_P(QueueTest, list_queue_backoff){
    QueueTest::BackoffTest<bk_conq::list_queue<queue_test_type_t>, queue_test_type_t>(_params);
}

TEST_P(QueueTest, array_queue_backoff){
    QueueTest::BackoffTest<bk_conq::array_queue<queue_test_type_t, BOUNDED_QUEUE_SIZE>, queue_test_type_t>(_params);
}

TEST_P(QueueTest, vector_queue_backoff){
    QueueTest::BackoffTest<bk_conq::vector_queue<queue_test_type_t>, queue_test_type_t>(_params, BOUNDED_QUEUE_SIZE);
}

TEST_P(QueueTest, cache_queue_backoff){
    QueueTest::BackoffTest<bk_conq::cache_queue<queue_test_type_t, BOUNDED_QUEUE_SIZE>, queue_test_type_t>(_params);
}

TEST_P(QueueTest, multilist_array_queue_backoff){
    QueueTest::BackoffTest<bk_conq::multilist_array_queue<queue_test_type_t, SUBQUEUE_SIZE>, queue_test_type_t>(_params);
}

TEST_P(QueueTest, multilist_vector_queue_backoff) {
	QueueTest::BackoffTest<bk_conq::multilist_vector_queue<queue_test_type_t>, queue_test_type_t>(_params, SUBQUEUE_SIZE);
}

TEST_P(QueueTest, list_queue_blocking) {
	QueueTest::BlockingTest<bk_conq::blocking_unbounded_queue<bk_conq::list_queue<queue_test_type_t> >, queue_test_type_t>(_params);
}

TEST_P(QueueTest, cache_queue_blocking) {
	QueueTest::BlockingTest<bk_conq::blocking_unbounded_queue<bk_conq::cache_queue<queue_test_type_t, BOUNDED_QUEUE_SIZE>>, queue_test_type_t>(_params);
}

TEST_P(QueueTest, multilist_array_queue_blocking) {
	QueueTest::BlockingTest<bk_conq::blocking_unbounded_queue<bk_conq::multilist_array_queue<queue_test_type_t, BOUNDED_QUEUE_SIZE>>, queue_test_type_t>(_params);
}

TEST_P(QueueTest, multilist_vector_queue_blocking) {
	QueueTest::BlockingTest<bk_conq::blocking_unbounded_queue<bk_conq::multilist_vector_queue<queue_test_type_t>>, queue_test_type_t>(_params, SUBQUEUE_SIZE);
}

TEST_P(QueueTest, array_queue_blocking) {
	QueueTest::BlockingTest<bk_conq::blocking_bounded_queue<bk_conq::array_queue<queue_test_type_t, SUBQUEUE_SIZE>>, queue_test_type_t>(_params);
}

TEST_P(QueueTest, vector_queue_blocking) {
	QueueTest::BlockingTest<bk_conq::blocking_bounded_queue<bk_conq::vector_queue<queue_test_type_t>>, queue_test_type_t>(_params, SUBQUEUE_SIZE);
}

INSTANTIATE_TEST_CASE_P(
        queue_benchmark,
        QueueTest,
        testing::Combine(
        Values(1, 2, 4, 8, 16, 32, 256), //readers
        Values(1, 2, 4, 8, 16, 32, 256), //writers
        Values(1e6, 1e7, 1e8, 1e9) //elements
        )
        );