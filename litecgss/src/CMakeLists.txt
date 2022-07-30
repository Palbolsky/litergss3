cmake_minimum_required(VERSION 3.9)
project(LiteCGSS_engine)

include_directories(src)
include_directories(external/skalog/src/src)

link_directories(../lib)

file(GLOB_RECURSE SOURCES
	src/*.h
	src/*.cpp
	${LITE_CGSS_EXTERNAL}/lodepng/*.cpp
	${LITE_CGSS_EXTERNAL}/lodepng/*.h
	${LITE_CGSS_EXTERNAL}/libnsgif/*.c
	${LITE_CGSS_EXTERNAL}/libnsgif/*.h
	${LITE_CGSS_EXTERNAL}/libnsgif/*.hpp
)

string(REGEX REPLACE "[^;]*CMakeFiles/[^;]+;?" "" SOURCES "${SOURCES}")

generate_source_groups("${SOURCES}" "${LITECGSS_ROOT_DIR}")

add_library(LiteCGSS_engine ${SOURCES})

set(TARGET_LIBS skalog sfml-graphics sfml-window sfml-system physfs)
target_link_libraries(LiteCGSS_engine ${TARGET_LIBS})
target_include_directories(LiteCGSS_engine
	PUBLIC ${LITE_CGSS_EXTERNAL}/lodepng
	PUBLIC ${LITE_CGSS_EXTERNAL}/libnsgif
	PUBLIC ${FETCHCONTENT_BASE_DIR}/physfs-src/src
	PUBLIC ../external/skalog/src/src)
