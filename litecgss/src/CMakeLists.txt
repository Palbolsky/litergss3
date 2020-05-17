cmake_minimum_required(VERSION 3.9)
project(LiteCGSS_engine)

include_directories(src)

file(GLOB_RECURSE SOURCES
	src/*.h
	src/*.cpp
	${LITE_CGSS_EXTERNAL}/lodepng/*.cpp
	${LITE_CGSS_EXTERNAL}/lodepng/*.h
)

string(REGEX REPLACE "[^;]*CMakeFiles/[^;]+;?" "" SOURCES "${SOURCES}")

generate_source_groups("${SOURCES}" "${LITECGSS_ROOT_DIR}")

add_library(LiteCGSS_engine ${SOURCES})

set(TARGET_LIBS sfml-graphics sfml-window sfml-system)
target_link_libraries(LiteCGSS_engine ${TARGET_LIBS})
target_include_directories(LiteCGSS_engine
	PUBLIC ${LITE_CGSS_EXTERNAL}/lodepng)
