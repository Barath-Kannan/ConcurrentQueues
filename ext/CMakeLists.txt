if(NOT TARGET gtest)
	add_subdirectory( googletest )
endif()

if(BENCHMARK_EXTERNAL)
	add_subdirectory( moodycamel )
endif()