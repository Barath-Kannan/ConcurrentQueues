################################################
# Project Settings
################################################
project(ConcurrentQueues CXX)
cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 14)

################################################
# Tests
################################################
enable_testing()
add_executable(tester
    test/basic_timer.cpp
    test/concurrent_queue_test.cpp
)
target_include_directories(tester
    PUBLIC test
    PUBLIC inc
)
target_link_libraries(tester
    GTest::gtest
    GTest::gtest_main
)

add_test(AllTestsInTester tester)
################################################
# Installation
################################################
install(DIRECTORY ${MPMCQueue_SOURCE_DIR}/inc/CONQ
    DESTINATION include)
