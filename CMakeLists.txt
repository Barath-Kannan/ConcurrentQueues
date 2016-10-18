cmake_minimum_required(VERSION 3.2)
project( ConcurrentQueues-distribution )

################################################
# Dependencies
################################################
include(ExternalProject)

##googletest##
set(GTEST_PATH ${CMAKE_CURRENT_BINARY_DIR}/googletest)
ExternalProject_Add(googletest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG release-1.8.0
    PREFIX ${GTEST_PATH}
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${GTEST_PATH}
    UPDATE_COMMAND ""
)
link_directories(${GTEST_PATH}/lib)

################################################
set(CMAKE_BUILD_TYPE Release)
add_subdirectory( ConcurrentQueues )
