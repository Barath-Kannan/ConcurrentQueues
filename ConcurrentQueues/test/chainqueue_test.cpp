#include "concurrent_queue_test.h"

namespace ChainQueue {
using qtype = bk_conq::chain_queue<QueueTest::queue_test_type_t>;
using mqtype = bk_conq::multi_unbounded_queue<qtype>;
using bqtype = bk_conq::blocking_unbounded_queue<qtype>;
using bmqtype = bk_conq::blocking_unbounded_queue<mqtype>;

TEST_P(QueueTest, chain_queue) {
    QueueTest::TemplatedTest<qtype, queue_test_type_t>(false);
}

TEST_P(QueueTest, chain_queue_blocking) {
    QueueTest::BlockingTest<bqtype, queue_test_type_t>(false);
}

TEST_P(QueueTest, multi_chain_queue) {
    QueueTest::TemplatedTest<mqtype, queue_test_type_t>(false, _params.subqueueSize);
}

TEST_P(QueueTest, multi_chain_queue_blocking) {
    QueueTest::BlockingTest<bmqtype, queue_test_type_t>(false, _params.subqueueSize);
}

TEST_P(QueueTest, chain_queue_prefill) {
    QueueTest::TemplatedTest<qtype, queue_test_type_t>(true);
}

TEST_P(QueueTest, chain_queue_blocking_prefill) {
    QueueTest::BlockingTest<bqtype, queue_test_type_t>(true);
}

TEST_P(QueueTest, multi_chain_queue_prefill) {
    QueueTest::TemplatedTest<mqtype, queue_test_type_t>(true, _params.subqueueSize);
}

TEST_P(QueueTest, multi_chain_queue_blocking_prefill) {
    QueueTest::BlockingTest<bmqtype, queue_test_type_t>(true, _params.subqueueSize);
}

}