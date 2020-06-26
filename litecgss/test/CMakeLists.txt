cmake_minimum_required(VERSION 3.9)
project(LiteCGSS_test)

include_directories(../src/src/LiteCGSS)
include_directories(../src/src)
include_directories(${LITE_CGSS_EXTERNAL}/doctest)

file(GLOB_RECURSE SOURCES
	src/*.h
	src/*.cpp
)

string(REGEX REPLACE "[^;]*CMakeFiles/[^;]+;?" "" SOURCES "${SOURCES}")

add_definitions(-DLITE_CGSS_TEST_DIR="${LITECGSS_ROOT_DIR}/test")

generate_source_groups("${SOURCES}" "${LITECGSS_ROOT_DIR}\\test\\src")

add_executable(LiteCGSS_test ${SOURCES})

set(TARGET_LIBS LiteCGSS_engine)
target_link_libraries(LiteCGSS_test ${TARGET_LIBS})
