################################################
# Project Settings
################################################
project(ConcurrentQueues CXX)
cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 14)

################################################
# Files
################################################
set(MAIN_HEADERS
	inc/bk_conq/array_queue.hpp
	inc/bk_conq/bounded_queue.hpp
	inc/bk_conq/cache_queue.hpp
	inc/bk_conq/list_queue.hpp
	inc/bk_conq/multilist_array_queue.hpp
	inc/bk_conq/multilist_vector_queue.hpp
	inc/bk_conq/multiarray_queue.hpp
	inc/bk_conq/multivector_queue.hpp
	inc/bk_conq/unbounded_queue.hpp
	inc/bk_conq/blocking_unbounded_queue.hpp
	inc/bk_conq/blocking_bounded_queue.hpp
	inc/bk_conq/vector_queue.hpp
)

set(TEST_HEADERS
	test/basic_timer.h
	test/concurrent_queue_test.h
)

set(TEST_SOURCES
    test/basic_timer.cpp
    test/concurrent_queue_test.cpp
)

source_group(main\\headers FILES ${MAIN_HEADERS})
source_group(test\\headers FILES ${TEST_HEADERS})
source_group(test\\sources FILES ${TEST_SOURCES})

################################################
# Targets
################################################

##################
# Testing Target #
##################
enable_testing(${PROJECT_NAME}_tester)
add_executable(${PROJECT_NAME}_tester
	${MAIN_HEADERS}
	${TEST_HEADERS}
	${TEST_SOURCES}
)
target_include_directories(${PROJECT_NAME}_tester
    PUBLIC test
    PUBLIC inc
)
target_link_libraries(${PROJECT_NAME}_tester
    gtest
    gtest_main
)

add_test(AllTestsInTester ${PROJECT_NAME}_tester)
