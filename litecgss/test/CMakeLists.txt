cmake_minimum_required(VERSION 3.9)
project(LiteCGSS_test)

add_compile_options("--coverage")
add_link_options("--coverage")
FIND_PROGRAM( GCOVR_PATH gcovr )

SET(GTEST_INSTALL_DIR ${LITECGSS_ROOT_DIR}/external/gtest)

# Download and unpack googletest at configure time
configure_file(${CMAKE_CURRENT_LIST_DIR}/external/CMakeLists.txt.in ${GTEST_INSTALL_DIR}/download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
	RESULT_VARIABLE result
	WORKING_DIRECTORY ${GTEST_INSTALL_DIR}/download )
if(result)
	message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
	RESULT_VARIABLE result
	WORKING_DIRECTORY ${GTEST_INSTALL_DIR}/download )
if(result)
	message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

include_directories(../src/src/LiteCGSS)
include_directories(../src/src)

add_definitions(-DLITE_CGSS_TEST_DIR="${LITECGSS_ROOT_DIR}/test")

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

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
SUBDIRLIST(SUBDIRS "${ROOT_TEST_SUBDIR}")
string(REGEX REPLACE "CMakeFiles;" "" SUBDIRS "${SUBDIRS}")
add_subdirectory(${ROOT_TEST_SUBDIR})