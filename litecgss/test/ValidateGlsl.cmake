# Compiles every shader of a folder of preprocessor golden files with
# glslangValidator (the Khronos reference compiler), so a GLSL ES 3.00 output
# is known to be valid GLSL ES 3.00 without a GL ES device.
#
#   cmake -DVALIDATOR=<glslangValidator> -DDIR=<golden folder> -P ValidateGlsl.cmake
#
# <case>.frag.expected / <case>.vert.expected: the stage comes from the name.

file(GLOB _shaders "${DIR}/*.expected")
if(NOT _shaders)
	message(FATAL_ERROR "No golden shader in ${DIR}")
endif()

set(_failed 0)
foreach(_shader IN LISTS _shaders)
	get_filename_component(_name "${_shader}" NAME)
	if(_name MATCHES "\\.frag\\.expected$")
		set(_stage frag)
	elseif(_name MATCHES "\\.vert\\.expected$")
		set(_stage vert)
	else()
		continue()
	endif()
	execute_process(COMMAND "${VALIDATOR}" -S ${_stage} "${_shader}"
		RESULT_VARIABLE _result OUTPUT_VARIABLE _output ERROR_VARIABLE _output)
	if(NOT _result EQUAL 0)
		message("${_name}:\n${_output}")
		math(EXPR _failed "${_failed} + 1")
	endif()
endforeach()

list(LENGTH _shaders _count)
if(_failed GREATER 0)
	message(FATAL_ERROR "${_failed} of ${_count} shaders rejected by glslangValidator")
endif()
message(STATUS "${_count} shaders accepted by glslangValidator")
