cmake_minimum_required(VERSION 3.9)
project(LiteCGSS_test)

FOREACH(subdir ${SUBDIRS})
	include(${subdir}/cmake.conf)
	file(GLOB_RECURSE SOURCES
		${subdir}/*.h
		${subdir}/*.cpp
	)
	string(REGEX REPLACE "[^;]*CMakeFiles/[^;]+;?" "" SOURCES "${SOURCES}")
	add_executable(LiteCGSS_${subdir}_test ${SOURCES} ${SUBDIR_SOURCES} ${MOCKS_SOURCES})

	set(TARGET_LIBS sfml-graphics sfml-window sfml-system ${SUBDIR_LIBS} gmock_main)
	target_link_libraries(LiteCGSS_${subdir}_test PRIVATE ${TARGET_LIBS})
ENDFOREACH()
