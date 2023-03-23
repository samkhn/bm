cmake_minimum_required(VERSION 3.5)

project(BM
	VERSION 0.1
	DESCRIPTION "Benchmarking library"
	LANGUAGES CXX)

add_library(bm STATIC src/bm.cc)
target_include_directories(bm
PUBLIC
	include/bm
)

add_executable(bm_main_test src/bm_main_test.cc)
target_link_libraries(bm_main_test PUBLIC bm)

add_custom_target(check-all
	DEPENDS bm_main_test
	COMMAND ./bm_main_test
)
