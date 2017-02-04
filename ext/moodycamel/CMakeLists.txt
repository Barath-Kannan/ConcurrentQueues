project(moodycamel_builder C CXX)

################################################################
# Load
################################################################
set(MOODYCAMEL_PATH ${CMAKE_BINARY_DIR}/ext/moodycamel)
if(NOT EXISTS ${MOODYCAMEL_PATH}/src)
	execute_process(
		COMMAND git clone https://github.com/cameron314/concurrentqueue.git ${MOODYCAMEL_PATH}/src
	)
endif()

add_library(moodycamel INTERFACE)
target_include_directories(moodycamel
	INTERFACE ${MOODYCAMEL_PATH}/src
)
