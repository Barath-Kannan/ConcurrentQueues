cmake_minimum_required(VERSION 3.2)
project( ConcurrentQueues-distribution )
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(BENCHMARK_EXTERNAL "BENCHMARK_EXTERNAL" OFF)

add_subdirectory( ext )
add_subdirectory( ConcurrentQueues )
