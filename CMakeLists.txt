cmake_minimum_required(VERSION 3.5)

project(BM
	VERSION 0.1
	DESCRIPTION "Header only nanosecond resolution C++ benchmarking library"
	LANGUAGES CXX)

add_library(bm INTERFACE)
target_include_directories(bm
INTERFACE
	include/bm
)

# Tests
add_executable(test_register tests/test_register.cc)
target_link_libraries(test_register PUBLIC bm)

add_custom_target(check-all
	DEPENDS test_register
	COMMAND ./test_register
)
