# ConcurrentQueues

This library aims to provide a variety of different multi-producer multi-consumer queue implementations for usage in concurrent contexts. The queues provided are highly configurable for adapting to different usage contexts (single producer, single consumer, high write contention, high read contention). The aim is not to achieve complete lock freedom but to achieve the highest speed. When the correct queue is used and is configured to match the context of their usage, the queue can achieve linear speedup in the number of threads (up to the number of cores) both in enqueue in and dequeue operations by converging on a state of near zero contention.

##Table of Contents
- [ConcurrentQueues](#concurrentqueues)
    - [Table of Contents](#table-of-contents)
    - [Queue Types](#queue-types)
    - [Building](#building)
    - [Usage](#usage)
    - [Performance](#performance)
	
##Queue types

There are 3 base queue types provided:
- Vector based bounded queue (bk_conq::vector_queue<T>)
- Linked list based unbounded queue (bk_conq::list_queue<T>)
- Linked list based bounded queue (bk_conq::bounded_list_queue<T>)

There are also 2 different types of queue adapters.
The subqueue adapters, which are used to increase performance with a large number of writers:
- Multi bounded queue (bk_conq::multi_bounded_queue<Q<T>>)
- Multi unbounded queue (bk_conq::multi_unbounded_queue<Q<T>>)

And also the blocking adapters, which provide a blocking interface.:
- Blocking bounded queue (bk_conq::blocking_bounded_queue<Q<T>>)
- Blocking unbounded queue (bk_conq::blocking_unbounded_queue<Q<T>>)

##Building

The queues are all header only, so no installation is required. The test cases can be built using cmake. 
```
    cmake -Bbuild -H.
	cmake --build build --config release
```
Cmake will pull in gtest from git in order to build the tests. To enable the external benchmark tests to be pulled in, perform the generation with the BENCHMARK_EXTERNAL flag set.

```
    cmake -Bbuild -H. -DBENCHMARK_EXTERNAL=ON
	cmake --build build --config release
```
This will pull in the moodycamel MPMC queue for comparison.

##Usage

The base queues are all templated on type type to be queued. Below is an example using the list queue.
```
    int x = 0;
	//unbounded list based queue
	//allocates as required
	bk_conq::list_queue<int> lq;
	
	//enqueue x
	vq.mp_enqueue(x);
	
	//dequeue into x, return true if item dequeued
	bool ret = vq.mc_dequeue(x);
```

The bounded queue types return bool on enqueue operations.
```
	int x = 0;
	size_t queue_size = 256;
	//bounded linked list based queue
	//allocates the required space upfront
	bk_conq::bounded_list_queue<int> lq(queue_size);
	
	//the vector queue uses the Vyukov MPMC queue design.
	//the subqueue size must therefore be a power of 2
	bk_conq::vector_queue<int> vq(queue_size);
	
	//enqueues will return false when the queue is full
	bool ret = lq.mp_enqueue(x);
	ret = vq.mp_enqueue(x);
	ret = lq.mc_dequeue(x);
	ret = vq.mc_dequeue(x);
```
The multi queue types have the same interface as the base queue types but their constructors require the user to specify the number of subqueues that will be used. It's generally recommended that the number of subqueues is equal to the expected number of writers.
```
	size_t queue_size = 256;
	size_t nsubqueues = 16;
	bk_conq::multi_unbounded_queue<list_queue<int>> mlq(nsubqueues);
	bk_conq::multi_bounded_queue<unbounded_list_queue<int>> mlq(queue_size, nsubqueues);
	bk_conq::multi_bounded_queue<vector_queue<int>> mlq(queue_size, nsubqueues);
```

## Performance

Below are some preliminary results with comparisons to Cameron Desrochers moody camel queue. All tests are conducted using a machine with an intel core i7-6700K @ 4.00GHz, and 16GB of RAM, compiled using the msvc-14.0 compiler (Visual Studio 2015).

| Fixed parameters: |
| --- |
| Number of elements to enqueue/dequeue: 100,000,000 |
| Type: size_t |
| For bounded queues - queue size: 2097152 elements |
| For multi queues - subqueue size: 16 subqueues |

#### 1 Reader. 16 Writers.
All time values are in nanoseconds, lower is better.

| Queue type | avrg enqueue | wrst enqueue | avg dequeue | wrst dequeue |
| --- | --- | --- | --- | --- |
| list_queue | 98 | 116 | 123 | 123 |
| bounded_list_queue | 254 | 277 | 278 | 278 |
| vector_queue | 134 | 148 | 148 | 148 |
| multi_list_queue | 3.45 | 4.17 | 23.02 | 23.02 |
| multi_bounded_list_queue | 26 | 45 | 46 | 46 |
| multi_vector_queue | 16 | 27 | 28 | 28 |
| moody_queue | 2.92 | 3.38 | 42.13 | 42.13 |
| moody_queue_tokenized | 2.41 | 2.747 | 24.2773 | 24.2773 |

From the results, we can see that the multi list queue has slightly worse enqueue performance and significantly better dequeue performance than the moody queue, unless the moody queue has access to thread local tokens.

#### 16 Readers. 1 Writer.
All time values are in nanoseconds, lower is better.

| Queue type | avrg enqueue | wrst enqueue | avg dequeue | wrst dequeue |
| --- | --- | --- | --- | --- |
| list_queue | 24 | 24 | 41 | 47 |
| bounded_list_queue | 45 | 45 | 53 | 59 |
| vector_queue | 89 | 89 | 99 | 114 |
| multi_list_queue | 34 | 34 | 58 | 60 |
| multi_bounded_list_queue | 70 | 70 | 72 | 77 |
| multi_vector_queue |  85 | 85 | 87 | 107 |
| moody_queue | 135 | 135 | 121 | 138.3 |
| moody_queue_tokenized | 141 | 141 | 128 | 144 |

As expected, any subqueue based scheme suffers significantly as only 1 subqueue is ever populated given there is only a single writer. The moody queue shows that it is quite expensive when no hit occurs on a dequeue. Wherever there is a single writer situation, the simple queue types should be preffered.

#### 16 Readers. 16 Writers.
All time values are in nanoseconds, lower is better.

| Queue type | avrg enqueue | wrst enqueue | avg dequeue | wrst dequeue |
| --- | --- | --- | --- | --- |
| list_queue | 40 | 42 | 45 | 55 |
| bounded_list_queue | 86 | 92 | 85 | 92 |
| vector_queue | 89 | 100 | 85 | 100 |
| multi_list_queue | 6.09 | 6.7 | 6.46 | 7.38 |
| multi_bounded_list_queue | 5.62 | 6.89 | 6.4 | 7.45 |
| multi_vector_queue |  4.27 | 6.13 | 5.28 | 6.89 |
| moody_queue | 6.18 | 9.9 | 42.87 | 47.82 |
| moody_queue_tokenized | 4.81 | 5.60 | 9.51 | 15.42 |

Where queues are under high contention and the queues rarely stay empty or full, the multi bounded variants exhibited significant performance gains over the other types. The moody queue performance falls off as the contention increases but given an equal number of readers and writers, the multi queues are able to maintain linear speed up up to the number of cores that are present. As far as I am aware, the performance of all the multi queue variants in this high contention scenario are the best of any MPMC queue implementation that currently exists publicly.
