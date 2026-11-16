cmake_minimum_required(VERSION 3.10)
project(LiteCGSS_test)

add_compile_options("--coverage")
add_link_options("--coverage")
FIND_PROGRAM( GCOVR_PATH gcovr )

include_directories(${LITECGSS_ROOT_DIR}/include)
include_directories(${LITECGSS_ROOT_DIR}/src)

add_definitions(-DLITE_CGSS_TEST_DIR="${LITECGSS_ROOT_DIR}/test")

# Google Test — prefer the system install (apt install libgtest-dev on
# Debian/Ubuntu). Fall back to FetchContent if not installed.
find_package(GTest QUIET)
if(NOT GTest_FOUND)
  message(STATUS "GTest not found system-wide; fetching googletest via FetchContent")
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

# Per-project CMakeLists link `gmock_main` unprefixed. Create an alias so
# both the system-installed (GTest::gmock_main) and the FetchContent paths
# expose the same target name.
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