cmake_minimum_required(VERSION 3.10)
project(LiteCGSS_test)

FOREACH(subdir ${SUBDIRS})
	include(${subdir}/cmake.conf)

	if(NOT("${SUBDIR_LIBS}" MATCHES "[^ ]?physfs[$ ]?") OR LITECGSS_USE_PHYSFS)
		file(GLOB_RECURSE SOURCES
			${subdir}/*.h
			${subdir}/*.cpp
		)
		string(REGEX REPLACE "[^;]*CMakeFiles/[^;]+;?" "" SOURCES "${SOURCES}")
		add_executable(LiteCGSS_${subdir}_test ${SOURCES} ${SUBDIR_SOURCES} ${MOCKS_SOURCES})

		set(TARGET_LIBS sfml-graphics sfml-window sfml-system ${SUBDIR_LIBS} gmock_main)
		if (LITECGSS_USE_PHYSFS)
			set(TARGET_LIBS ${TARGET_LIBS} physfs)
		endif()
		target_link_libraries(LiteCGSS_${subdir}_test PRIVATE ${TARGET_LIBS})
	else()
		message(STATUS "Not building ${subdir} because physfs is disabled and it depends on it.")
	endif()
ENDFOREACH()
