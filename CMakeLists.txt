cmake_minimum_required(VERSION 3.2)
project( ConcurrentQueues-distribution )

add_subdirectory( 3rdParty )
link_directories( ${EXT_LIB_DIRS} )

add_subdirectory( ConcurrentQueues )