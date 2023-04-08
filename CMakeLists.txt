cmake_minimum_required(VERSION 3.5)

project(BM
	VERSION 0.1
	DESCRIPTION "Header only nanosecond resolution C++ benchmarking library"
	LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)

add_library(bm INTERFACE)
target_include_directories(bm INTERFACE .)

add_custom_target(lint
	clang-tidy --format-style='Google' -checks=cert-* --warnings-as-errors=* ${CMAKE_SOURCE_DIR}/bm.hpp
	DEPENDS bm
)

# Tests
# For development/testing you'll need Python3 and a C++ compiler that supports at least C++11

add_executable(test-register tests/test_register.cc)
target_link_libraries(test-register PUBLIC bm)
add_custom_target(check-register
	$<TARGET_FILE:test-register>
	DEPENDS test-register
)

add_executable(test-flags tests/test_flags.cc)
target_link_libraries(test-flags PUBLIC bm)
add_custom_target(check-flags
	python3 ${CMAKE_SOURCE_DIR}/tests/test_flags_integration.py $<TARGET_FILE:test-flags>
	DEPENDS test-flags
)

add_executable(test-sysfs-scan tests/test_sysfs_scan.cc)
target_link_libraries(test-sysfs-scan PUBLIC bm)
add_custom_target(check-sysfs-scan
	python3 ${CMAKE_SOURCE_DIR}/tests/test_sysfs_scan_integration.py $<TARGET_FILE:test-sysfs-scan>
	DEPENDS test-sysfs-scan
)

add_executable(test-output tests/test_output.cc)
target_link_libraries(test-output PUBLIC bm)
add_custom_target(check-output
	python3 ${CMAKE_SOURCE_DIR}/tests/test_output_flag_integration.py $<TARGET_FILE:test-output>
	DEPENDS test-output
)

add_custom_target(check-all
	DEPENDS
		check-register
		check-flags
		check-sysfs-scan
    check-output
)

