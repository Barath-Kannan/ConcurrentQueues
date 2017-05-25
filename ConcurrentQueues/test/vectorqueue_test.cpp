#include "concurrent_queue_test.h"

namespace VectorQueue {
using qtype = bk_conq::vector_queue<QueueTest::queue_test_type_t>;
using mqtype = bk_conq::multi_bounded_queue<qtype>;
using bqtype = bk_conq::blocking_bounded_queue<qtype>;
using bmqtype = bk_conq::blocking_bounded_queue<mqtype>;

TEST_P(QueueTest, vector_queue) {
    QueueTest::TemplatedTest<qtype, queue_test_type_t>();
}

TEST_P(QueueTest, vector_queue_blocking) {
    QueueTest::BlockingTest<bqtype, queue_test_type_t>();
}

TEST_P(QueueTest, multi_vector_queue) {
    QueueTest::TemplatedTest<mqtype, queue_test_type_t>(_params.subqueueSize);
}

TEST_P(QueueTest, multi_vector_queue_blocking) {
    QueueTest::BlockingTest<bmqtype, queue_test_type_t>(_params.subqueueSize);
}

}