project(gtest_builder C CXX)

################################################################
# Options
################################################################
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(gtest_disable_pthreads OFF CACHE BOOL "" FORCE)
if(MINGW)
    set(gtest_disable_pthreads ON CACHE BOOL "" FORCE)
endif()

set(GTEST_PATH ${CMAKE_BINARY_DIR}/ext/googletest)
if(NOT EXISTS ${GTEST_PATH}/src)
	execute_process(
		COMMAND git clone -b release-1.8.0 https://github.com/google/googletest ${GTEST_PATH}/src
	)
endif()
if(NOT TARGET gtest)
	add_subdirectory(${GTEST_PATH}/src ${GTEST_PATH}/build)
endif()
