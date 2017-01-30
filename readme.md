# ConcurrentQueues

This library aims to provide a variety of different multi-producer multi-consumer queue implementations for usage in concurrent contexts. The queues provided are highly configurable for adapting to different usage contexts (single producer, single consumer, high write contention, high read contention). The aim is not to achieve complete lock freedom but to achieve the highest speed and minimize the effects of contention. When the correct queue is used and is configured to match the context of their usage, the queues can always perform enqueue operations and dequeue operations at an average frequency of 10 nanoseconds or faster by minimizing sharing/false-sharing and limiting points of contention.

Todo: 
* convert multiqueue variants to generic multiqueue template
* remove array-based queues; static polymorphism on array/subqueue size gives only minor performance improvements on some queue types
* after removing array-based queue, add test combiner for subqueue and bounded size
