################################################
# Project Settings
################################################
project(ConcurrentQueues CXX)
cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 14)

################################################
# Tests
################################################
add_executable(tester
    test/basic_timer.cpp
    test/concurrent_queue_test.cpp
)
add_dependencies(tester googletest)
target_include_directories(tester
    PUBLIC test
    PUBLIC inc
    PUBLIC ${GTEST_PATH}/include
)
target_link_libraries(tester
    pthread
    gtest
    gtest_main
)

################################################
# Installation
################################################
install(DIRECTORY ${MPMCQueue_SOURCE_DIR}/inc/CONQ
    DESTINATION include)
