project(gtest_builder C CXX)
include(ExternalProject)

set(GTEST_FORCE_SHARED_CRT ON)
set(GTEST_DISABLE_PTHREADS OFF)
if(MINGW)
    set(GTEST_DISABLE_PTHREADS ON)
endif()

set(GTEST_PATH ${CMAKE_CURRENT_BINARY_DIR})

ExternalProject_Add(googletest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG release-1.8.0
    PREFIX ${GTEST_PATH}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${GTEST_PATH}
        -DCMAKE_CXX_FLAGS=${MSVC_COMPILER_DEFS}
        -Dgtest_force_shared_crt=${GTEST_FORCE_SHARED_CRT}
        -Dgtest_disable_pthreads=${GTEST_DISABLE_PTHREADS}
        -DBUILD_GTEST=ON
    UPDATE_COMMAND ""
)

set(GTEST_INCLUDE_DIRS ${GTEST_PATH}/include PARENT_SCOPE)
set(GTEST_LIB_DIRS ${GTEST_PATH}/lib PARENT_SCOPE)