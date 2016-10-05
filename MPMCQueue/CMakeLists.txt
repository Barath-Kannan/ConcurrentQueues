project(MPMCQueue CXX)
cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 14)

################################################
# Testing
################################################
add_subdirectory(ext/googletest)
add_executable(tester
    test/BasicTimer.cpp
    test/MPMCQueueTest.cpp
)
target_include_directories(tester
    PUBLIC test
    PUBLIC inc
    PUBLIC ext/googletest/googletest/include
)
target_link_libraries(tester
    PUBLIC pthread
    PUBLIC gtest_main_shared
)

