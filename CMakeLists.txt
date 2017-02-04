cmake_minimum_required(VERSION 3.2)
project( ConcurrentQueues-distribution )

option(BENCHMARK_EXTERNAL "BENCHMARK_EXTERNAL" OFF)

add_subdirectory( ext )
add_subdirectory( ConcurrentQueues )
