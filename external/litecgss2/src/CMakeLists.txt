cmake_minimum_required(VERSION 3.10)
project(LiteCGSS2_engine)

include_directories(src)
include_directories(external/skalog/src/src)

link_directories(../lib)

file(GLOB_RECURSE SOURCES
	src/*.h
	src/*.cpp
	${LITE_CGSS_EXTERNAL}/lodepng/*.cpp
	${LITE_CGSS_EXTERNAL}/lodepng/*.h
)

string(REGEX REPLACE "[^;]*CMakeFiles/[^;]+;?" "" SOURCES "${SOURCES}")

generate_source_groups("${SOURCES}" "${LITECGSS_ROOT_DIR}")

add_library(LiteCGSS2_engine ${SOURCES})

set(TARGET_LIBS skalog raylib)
if (LITECGSS_USE_PHYSFS)
	set(TARGET_LIBS ${TARGET_LIBS} physfs)
	set_property(TARGET physfs-static PROPERTY POSITION_INDEPENDENT_CODE ON)
	set(LITECGSS_PHYSFS_TARGET_INCLUDE_DIR 	PUBLIC ${FETCHCONTENT_BASE_DIR}/physfs-src/src)
endif()

target_link_libraries(LiteCGSS2_engine ${TARGET_LIBS})
target_include_directories(LiteCGSS2_engine
	PUBLIC ${LITE_CGSS_EXTERNAL}/lodepng
	${LITECGSS_PHYSFS_TARGET_INCLUDE_DIR}
	PUBLIC ../external/skalog/src/src)
if (NOT BUILD_SHARED_LIBS)
	set_property(TARGET LiteCGSS2_engine PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()
