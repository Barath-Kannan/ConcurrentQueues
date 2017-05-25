################################################
# Project Settings
################################################
project(ConcurrentQueues CXX)
cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 14)

if (${CMAKE_PROJECT_NAME} MATCHES "ConcurrentQueues")
    option(CONCURRENTQUEUES_TESTS "CONCURRENTQUEUES_TESTS" ON)
    message(STATUS "ConcurrentQueues tests enabled")
else()
    option(CONCURRENTQUEUES_TESTS "CONCURRENTQUEUES_TESTS" OFF)
    message(STATUS "ConcurrentQueues tests disabled")
endif()

################################################
# Files
################################################
set(MAIN_HEADERS
    inc/bk_conq/bounded_queue.hpp
    inc/bk_conq/unbounded_queue.hpp
    inc/bk_conq/blocking_bounded_queue.hpp
    inc/bk_conq/blocking_unbounded_queue.hpp
    inc/bk_conq/multi_bounded_queue.hpp
    inc/bk_conq/multi_unbounded_queue.hpp
    inc/bk_conq/list_queue.hpp
    inc/bk_conq/vector_queue.hpp
    inc/bk_conq/bounded_list_queue.hpp
    inc/bk_conq/chain_queue.hpp
    inc/bk_conq/details/tlos.hpp
)

set(TEST_GENERAL_HEADERS
    test/basic_timer.h
    test/concurrent_queue_test.h
)

set(TEST_GENERAL_SOURCES
    test/basic_timer.cpp
    test/concurrent_queue_test.cpp
)

set(TEST_LISTQUEUE_SOURCES
    test/listqueue_test.cpp
)

set(TEST_CHAINQUEUE_SOURCES
    test/chainqueue_test.cpp
)

set(TEST_BOUNDEDLISTQUEUE_SOURCES
    test/boundedlistqueue_test.cpp
)

set(TEST_VECTORQUEUE_SOURCES
    test/vectorqueue_test.cpp
)

if(BENCHMARK_EXTERNAL)
    set(TEST_EXTERNAL_SOURCES
        test/moodycamel_test.cpp
    )
endif()

source_group(main\\headers FILES ${MAIN_HEADERS})
source_group(test\\headers FILES ${TEST_GENERAL_HEADERS})
source_group(test\\sources FILES ${TEST_GENERAL_SOURCES} ${TEST_LISTQUEUE_SOURCES} ${TEST_CHAINQUEUE_SOURCES} ${TEST_BOUNDEDLISTQUEUE_SOURCES} ${TEST_VECTORQUEUE_SOURCES} ${TEST_EXTERNAL_SOURCES})

################################################
# Targets
################################################

##################
# Testing Target #
##################
if(CONCURRENTQUEUES_TESTS)

    add_library(testlib
        ${MAIN_HEADERS}
        ${TEST_GENERAL_HEADERS}
        ${TEST_GENERAL_SOURCES}
    )
    target_include_directories(testlib
        PUBLIC test
        PUBLIC inc
    )
    target_link_libraries(testlib
        PUBLIC gtest
        PUBLIC gtest_main
    )
    set_target_properties(testlib PROPERTIES FOLDER bk_conq)
    
    add_executable(ListQueueTest
        ${TEST_LISTQUEUE_SOURCES}
    )
    target_link_libraries(ListQueueTest
        PUBLIC testlib
    )
    set_target_properties(ListQueueTest PROPERTIES FOLDER bk_conq)
    
    add_executable(ChainQueueTest
        ${TEST_CHAINQUEUE_SOURCES}
    )
    target_link_libraries(ChainQueueTest
        PUBLIC testlib
    )
    set_target_properties(ChainQueueTest PROPERTIES FOLDER bk_conq)
    
    add_executable(BoundedListQueueTest
        ${TEST_BOUNDEDLISTQUEUE_SOURCES}
    )
    target_link_libraries(BoundedListQueueTest
        PUBLIC testlib
    )
    set_target_properties(BoundedListQueueTest PROPERTIES FOLDER bk_conq)

    add_executable(VectorQueueTest
        ${TEST_VECTORQUEUE_SOURCES}
    )
    target_link_libraries(VectorQueueTest
        PUBLIC testlib
    )
    set_target_properties(VectorQueueTest PROPERTIES FOLDER bk_conq)
    
    if(BENCHMARK_EXTERNAL)
        add_executable(MoodyQueueTest
            ${TEST_EXTERNAL_SOURCES}
        )
        target_link_libraries(MoodyQueueTest
            PUBLIC testlib
            PUBLIC moodycamel
        )
        set_target_properties(MoodyQueueTest PROPERTIES FOLDER bk_conq)
    endif()
    
endif()
