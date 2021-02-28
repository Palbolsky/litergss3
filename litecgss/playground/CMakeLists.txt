cmake_minimum_required(VERSION 3.9)
project(LiteCGSS_playground)

include_directories(../src/src/LiteCGSS)
include_directories(../src/src)

file(GLOB_RECURSE SOURCES
	src/*.h
	src/*.cpp
)

string(REGEX REPLACE "[^;]*CMakeFiles/[^;]+;?" "" SOURCES "${SOURCES}")

add_definitions(-DLITE_CGSS_PLAYGROUND_DIR="${LITECGSS_ROOT_DIR}/playground")

generate_source_groups("${SOURCES}" "${LITECGSS_ROOT_DIR}\\playground\\src")

add_executable(LiteCGSS_playground ${SOURCES})

set(TARGET_LIBS LiteCGSS_engine sfml-graphics sfml-window sfml-system)
target_link_libraries(LiteCGSS_playground ${TARGET_LIBS})
