#include "concurrent_queue_test.h"
#include <iostream>
#include <chrono>
#include <type_traits>
#include <queue>
#include "concurrentqueue.h"

using ::testing::Values;
using ::testing::Combine;
using ::testing::Types;
using std::cout;
using std::endl;

namespace MoodyQueue{
	template<typename T>
	class MoodyQueueAdapter : public bk_conq::unbounded_queue<T, MoodyQueueAdapter<T>> {
		friend bk_conq::unbounded_queue<T, MoodyQueueAdapter<T>>;
	protected:
		template <typename R>
		void sp_enqueue_impl(R&& item) {
			_q.enqueue(std::forward<R>(item));
		}

		template <typename R>
		void mp_enqueue_impl(R&& item) {
			_q.enqueue(std::forward<R>(item));
		}
		
		bool sc_dequeue_impl(T& item) {
			return _q.try_dequeue(item);
		}

		bool mc_dequeue_impl(T& item) {
			return _q.try_dequeue(item);
		}

		bool mc_dequeue_uncontended(T& item) {
			return _q.try_dequeue(item);
		}

	private:
		moodycamel::ConcurrentQueue<T> _q;
	};

    template<typename T>
    class MoodyQueueTokenizedAdapter : public bk_conq::unbounded_queue<T, MoodyQueueTokenizedAdapter<T>> {
        friend bk_conq::unbounded_queue<T, MoodyQueueTokenizedAdapter<T>>;
    protected:
        template <typename R>
        void sp_enqueue_impl(R&& item) {
            thread_local moodycamel::ProducerToken token(_q);
            _q.enqueue(token, std::forward<R>(item));
        }

        template <typename R>
        void mp_enqueue_impl(R&& item) {
            thread_local moodycamel::ProducerToken token(_q);
            _q.enqueue(token, std::forward<R>(item));
        }

        bool sc_dequeue_impl(T& item) {
            thread_local moodycamel::ConsumerToken token(_q);
            return _q.try_dequeue(token, item);
        }

        bool mc_dequeue_impl(T& item) {
            thread_local moodycamel::ConsumerToken token(_q);
            return _q.try_dequeue(token, item);
        }

        bool mc_dequeue_uncontended_impl(T& item) {
            return mc_dequeue(item);
        }

    private:
        moodycamel::ConcurrentQueue<T> _q;
    };

    struct CQT : public moodycamel::ConcurrentQueueDefaultTraits {
        static const size_t MAX_SUBQUEUE_SIZE = 33554432;
    };

    template<typename T>
    class MoodyQueueBoundedAdapter : public bk_conq::bounded_queue<T, MoodyQueueBoundedAdapter<T>> {
        friend bk_conq::bounded_queue<T, MoodyQueueBoundedAdapter<T>>;
    protected:
        template <typename R>
        bool sp_enqueue_impl(R&& item) {
            return _q.enqueue(std::forward<R>(item));
        }

        template <typename R>
        bool mp_enqueue_impl(R&& item) {
            return _q.enqueue(std::forward<R>(item));
        }

        bool sc_dequeue_impl(T& item) {
            return _q.try_dequeue(item);
        }

        bool mc_dequeue_impl(T& item) {
            return _q.try_dequeue(item);
        }

        bool mc_dequeue_uncontended(T& item) {
            return _q.try_dequeue(item);
        }

    private:

        moodycamel::ConcurrentQueue<T, CQT> _q;
    };

    template<typename T>
    class MoodyQueueBoundedTokenizedAdapter : public bk_conq::bounded_queue<T, MoodyQueueBoundedTokenizedAdapter<T>> {
        friend bk_conq::bounded_queue<T, MoodyQueueBoundedTokenizedAdapter<T>>;
    protected:
        template <typename R>
        bool sp_enqueue_impl(R&& item) {
            thread_local moodycamel::ProducerToken token(_q);
            return _q.enqueue(token, std::forward<R>(item));
        }

        template <typename R>
        bool mp_enqueue_impl(R&& item) {
            thread_local moodycamel::ProducerToken token(_q);
            return _q.enqueue(token, std::forward<R>(item));
        }

        bool sc_dequeue_impl(T& item) {
            thread_local moodycamel::ConsumerToken token(_q);
            return _q.try_dequeue(token, item);
        }

        bool mc_dequeue_impl(T& item) {
            thread_local moodycamel::ConsumerToken token(_q);
            return _q.try_dequeue(token, item);
        }

        bool mc_dequeue_uncontended_impl(T& item) {
            return mc_dequeue(item);
        }

    private:
        moodycamel::ConcurrentQueue<T, CQT> _q;
    };

	using qtype = MoodyQueueAdapter<QueueTest::queue_test_type_t>;
	using tqtype = MoodyQueueTokenizedAdapter<QueueTest::queue_test_type_t>;
    using boundqtype = MoodyQueueBoundedAdapter<QueueTest::queue_test_type_t>;
    using boundtqtype = MoodyQueueBoundedTokenizedAdapter<QueueTest::queue_test_type_t>;

    TEST_P(QueueTest, moody_queue) {
        QueueTest::TemplatedTest<qtype, queue_test_type_t>(false);
    }

    TEST_P(QueueTest, moody_queue_prefill) {
        QueueTest::TemplatedTest<qtype, queue_test_type_t>(true);
    }

    TEST_P(QueueTest, moody_queue_tokenized) {
        QueueTest::TemplatedTest<tqtype, queue_test_type_t>(false);
    }

    TEST_P(QueueTest, moody_queue_tokenized_prefill) {
        QueueTest::TemplatedTest<tqtype, queue_test_type_t>(true);
    }

    TEST_P(QueueTest, moody_queue_bounded) {
        QueueTest::TemplatedBoundedTest<boundqtype, queue_test_type_t>();
    }

    TEST_P(QueueTest, moody_queue_tokenized_bounded) {
        QueueTest::TemplatedBoundedTest<boundtqtype, queue_test_type_t>();
    }

}
