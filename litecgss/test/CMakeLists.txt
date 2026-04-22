cmake_minimum_required(VERSION 3.10)
project(LiteCGSS_test)

add_compile_options("--coverage")
add_link_options("--coverage")
FIND_PROGRAM( GCOVR_PATH gcovr )

SET(GTEST_INSTALL_DIR ${LITECGSS_ROOT_DIR}/external/gtest)

# Download and unpack googletest at configure time
configure_file(${CMAKE_CURRENT_LIST_DIR}/external/CMakeLists.txt.in ${GTEST_INSTALL_DIR}/download/CMakeLists.txt)

include_directories(${LITECGSS_ROOT_DIR}/include)
include_directories(${LITECGSS_ROOT_DIR}/src)

add_definitions(-DLITE_CGSS_TEST_DIR="${LITECGSS_ROOT_DIR}/test")

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${GTEST_INSTALL_DIR}/src
                 ${GTEST_INSTALL_DIR}/build
                 EXCLUDE_FROM_ALL)

MACRO(SUBDIRLIST result curdir)
	FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
	SET(dirlist "")
	FOREACH(child ${children})
		IF(IS_DIRECTORY ${curdir}/${child})
			LIST(APPEND dirlist ${child})
		ENDIF()
	ENDFOREACH()
	SET(${result} ${dirlist})
ENDMACRO()

SET(ROOT_TEST_SUBDIR "${CMAKE_CURRENT_LIST_DIR}/src/projects")
SET(MOCKS_DIR "${CMAKE_CURRENT_LIST_DIR}/src/mocks")
# Per-project cmake.conf files reference .cpp via LITECGSS_SRC_DIR and .h
# via LITECGSS_INCLUDE_DIR to match the public/private split in the engine.
SET(LITECGSS_SRC_DIR ${LITECGSS_ROOT_DIR}/src/LiteCGSS)
SET(LITECGSS_INCLUDE_DIR ${LITECGSS_ROOT_DIR}/include/LiteCGSS)

file(GLOB_RECURSE MOCKS_SOURCES
	${MOCKS_DIR}/*.h
	${MOCKS_DIR}/*.cpp
)
include_directories(${MOCKS_DIR})

SUBDIRLIST(SUBDIRS "${ROOT_TEST_SUBDIR}")
string(REGEX REPLACE "CMakeFiles;" "" SUBDIRS "${SUBDIRS}")
add_subdirectory(${ROOT_TEST_SUBDIR})