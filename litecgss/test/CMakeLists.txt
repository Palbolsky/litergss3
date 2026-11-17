cmake_minimum_required(VERSION 3.10)
project(LiteCGSS_test)

add_compile_options("--coverage")
add_link_options("--coverage")
FIND_PROGRAM( GCOVR_PATH gcovr )

include_directories(${LITECGSS_ROOT_DIR}/include)
include_directories(${LITECGSS_ROOT_DIR}/src)

add_definitions(-DLITE_CGSS_TEST_DIR="${LITECGSS_ROOT_DIR}/test")

# Google Test, provided (GTest_DIR / CMAKE_PREFIX_PATH / system: apt install
# libgtest-dev), never downloaded. tools/deps/build-deps.sh builds one into
# vendor/deps; or configure with -DLITECGSS_BUILD_TESTS=OFF.
find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
  find_package(GTest QUIET)
endif()
if(NOT TARGET GTest::gmock_main)
  message(FATAL_ERROR "LITECGSS_BUILD_TESTS: no GoogleTest (with GMock) found. Give its location "
    "(-DGTest_DIR=<prefix>/lib/cmake/GTest or CMAKE_PREFIX_PATH), build one with tools/deps/build-deps.sh, "
    "or -DLITECGSS_BUILD_TESTS=OFF")
endif()

# Per-project CMakeLists link `gmock_main` unprefixed.
if(TARGET GTest::gmock_main AND NOT TARGET gmock_main)
  add_library(gmock_main ALIAS GTest::gmock_main)
endif()

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
SET(LITECGSS_SRC_DIR ${LITECGSS_ROOT_DIR}/src)
SET(LITECGSS_INCLUDE_DIR ${LITECGSS_ROOT_DIR}/include/LiteCGSS)

file(GLOB_RECURSE MOCKS_SOURCES
	${MOCKS_DIR}/*.h
	${MOCKS_DIR}/*.cpp
)
include_directories(${MOCKS_DIR})

SUBDIRLIST(SUBDIRS "${ROOT_TEST_SUBDIR}")
string(REGEX REPLACE "CMakeFiles;" "" SUBDIRS "${SUBDIRS}")
add_subdirectory(${ROOT_TEST_SUBDIR})