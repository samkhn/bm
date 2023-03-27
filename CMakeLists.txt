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
# For development/testing you'll need Python3 and a C++ compiler that supports at least C++11

add_executable(test_register tests/test_register.cc)
target_link_libraries(test_register PUBLIC bm)
add_custom_target(check_register DEPENDS test_register)
add_custom_command(
	TARGET check_register
	COMMAND ./test_register
	COMMENT "Tests whether we can register a benchmark"
)

add_executable(test_flags tests/test_flags.cc)
target_link_libraries(test_flags PUBLIC bm)
add_custom_target(check_flags DEPENDS test_flags)
add_custom_command(
	TARGET check_flags
	COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/test_flags_integration.py $<TARGET_FILE:test_flags>
	COMMENT "Runs test_flag binary and checks whether flags are returning expected values"
)

add_executable(test_sysfs_scan tests/test_sysfs_scan.cc)
target_link_libraries(test_sysfs_scan PUBLIC bm)
add_custom_target(check_sysfs_scan DEPENDS test_sysfs_scan)
add_custom_command(
	TARGET check_sysfs_scan
	COMMAND python3 ${CMAKE_SOURCE_DIR}/tests/test_sysfs_scan_integration.py $<TARGET_FILE:test_sysfs_scan>
	COMMENT "Runs test_sysfs_scan: checks /proc /sysfs for relevant, important flags"
)

add_custom_target(check_all
	DEPENDS
		check_register
		check_flags
		check_sysfs_scan
)

