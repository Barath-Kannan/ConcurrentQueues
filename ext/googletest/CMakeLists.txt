project(gtest_builder C CXX)
include(ExternalProject)

################################################################
# Options
################################################################
set(GTEST_FORCE_SHARED_CRT ON)
set(GTEST_DISABLE_PTHREADS OFF)
if(MINGW)
    set(GTEST_DISABLE_PTHREADS ON)
endif()

set(GTEST_PATH ${CMAKE_BINARY_DIR}/googletest)

################################################################
# External Project
################################################################
ExternalProject_Add(googletest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG release-1.8.0
    PREFIX ${GTEST_PATH}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${GTEST_PATH}
        -DCMAKE_CXX_FLAGS=${MSVC_COMPILER_DEFS}
        -DCMAKE_BUILD_TYPE=${CMAKE_CFG_INTDIR}
        -Dgtest_force_shared_crt=${GTEST_FORCE_SHARED_CRT}
        -Dgtest_disable_pthreads=${GTEST_DISABLE_PTHREADS}
        -DBUILD_GTEST=ON
    UPDATE_COMMAND ""
)

################################################################
# gtest library
################################################################
add_library(gtest INTERFACE)
add_library(GTest::gtest ALIAS gtest)
target_include_directories(gtest INTERFACE
    ${GTEST_PATH}/include
)
find_package(Threads)
target_link_libraries(gtest INTERFACE
    ${CMAKE_THREAD_LIBS_INIT}
    ${GTEST_PATH}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}
)
add_dependencies(gtest googletest)
################################################################
# gtest_main library
################################################################
add_library(gtest_main INTERFACE)
add_library(GTest::gtest_main ALIAS gtest_main)
target_include_directories(gtest_main INTERFACE
    ${GTEST_PATH}/include
)
target_link_libraries(gtest INTERFACE
    ${GTEST_PATH}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}
)
add_dependencies(gtest_main googletest)
