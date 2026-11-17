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
		# cmake.conf names PhysicsFS `physfs-static` (also the marker above):
		# the package's target found by litecgss/CMakeLists.txt.
		list(TRANSFORM SUBDIR_LIBS REPLACE "^physfs-static$" "${LITECGSS_PHYSFS_TARGET}")
		file(GLOB_RECURSE SOURCES
			${subdir}/*.h
			${subdir}/*.cpp
		)
		string(REGEX REPLACE "[^;]*CMakeFiles/[^;]+;?" "" SOURCES "${SOURCES}")
		add_executable(LiteCGSS_${subdir}_test ${SOURCES} ${SUBDIR_SOURCES} ${MOCKS_SOURCES})

		set(TARGET_LIBS LiteCGSS::config LiteCGSS_backend ${SUBDIR_LIBS} gmock_main)
		if (LITECGSS_USE_PHYSFS)
			set(TARGET_LIBS ${TARGET_LIBS} ${LITECGSS_PHYSFS_TARGET})
		endif()
		target_link_libraries(LiteCGSS_${subdir}_test PRIVATE ${TARGET_LIBS})
		add_test(NAME LiteCGSS.${subdir} COMMAND LiteCGSS_${subdir}_test)
	else()
		message(STATUS "Not building ${subdir} because physfs is disabled and it depends on it.")
	endif()
ENDFOREACH()

# The GLSL ES 3.00 golden files (ShaderPreprocessor, Target::RaylibGlslEs300)
# compiled by glslangValidator, the Khronos reference compiler, when installed
# (apt install glslang-tools): valid GLSL ES 3.00 without a GL ES device.
find_program(GLSLANG_VALIDATOR NAMES glslangValidator)
if(GLSLANG_VALIDATOR)
	add_test(NAME LiteCGSS.ShaderPreprocessor.glslang
		COMMAND ${CMAKE_COMMAND} -DVALIDATOR=${GLSLANG_VALIDATOR}
			-DDIR=${CMAKE_CURRENT_SOURCE_DIR}/ShaderPreprocessor/cases/raylib_es300
			-P ${LITECGSS_ROOT_DIR}/test/ValidateGlsl.cmake)
else()
	message(STATUS "glslangValidator not found: the GLSL ES 3.00 golden files are not compiled (apt install glslang-tools)")
endif()
