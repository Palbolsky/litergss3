cmake_minimum_required(VERSION 3.10)
project(LiteCGSS_test)

FOREACH(subdir ${SUBDIRS})
	# Reset per-iteration — CMake variables persist across FOREACH, so
	# leftover values from an earlier subdir's cmake.conf would leak into
	# subdirs that don't set them (e.g. Loaders' sfml-specific sources
	# leaking into Profiling under CGSS_BACKEND=raylib).
	SET(SUBDIR_SOURCES "")
	SET(SUBDIR_LIBS "")
	include(${subdir}/cmake.conf)

	if(NOT("${SUBDIR_LIBS}" MATCHES "[^ ]?physfs[$ ]?") OR LITECGSS_USE_PHYSFS)
		file(GLOB_RECURSE SOURCES
			${subdir}/*.h
			${subdir}/*.cpp
		)
		string(REGEX REPLACE "[^;]*CMakeFiles/[^;]+;?" "" SOURCES "${SOURCES}")
		add_executable(LiteCGSS_${subdir}_test ${SOURCES} ${SUBDIR_SOURCES} ${MOCKS_SOURCES})

		set(TARGET_LIBS LiteCGSS::config sfml-graphics sfml-window sfml-system ${SUBDIR_LIBS} gmock_main)
		if (LITECGSS_USE_PHYSFS)
			set(TARGET_LIBS ${TARGET_LIBS} physfs-static)
		endif()
		target_link_libraries(LiteCGSS_${subdir}_test PRIVATE ${TARGET_LIBS})
		add_test(NAME LiteCGSS.${subdir} COMMAND LiteCGSS_${subdir}_test)
	else()
		message(STATUS "Not building ${subdir} because physfs is disabled and it depends on it.")
	endif()
ENDFOREACH()
