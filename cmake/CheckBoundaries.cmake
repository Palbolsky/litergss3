# Module boundary check between LiteCGSS and LiteRGSS.
#
#   cmake -P cmake/CheckBoundaries.cmake      (or `rake lint`, or `ctest -R boundaries`)
#
# The build already makes most violations impossible (litecgss/src is not on
# LiteRGSS' include path, Ruby headers are not on LiteCGSS' one). This script
# guards what include paths cannot:
#
#   1. ext/LiteRGSS3  -> never names a backend directly: no raylib / SFML /
#                        GLFW header (they are transitively reachable through
#                        public engine headers), no #if on CGSS_BACKEND_*.
#   2. ext/LiteRGSS3  -> CMake never widens the engine interface: no
#                        litecgss/src path, no LiteCGSS::extension_spi.
#   3. litecgss       -> never sees Ruby nor LiteRGSS.
#   4. litecgss/include (public) -> never includes a private src/ header
#                        (except Extension/NativeAccess.h, the opt-in SPI).
#
# Exit code is non-zero when a violation is found.

cmake_minimum_required(VERSION 3.21)

get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(EXT_DIR      "${ROOT}/ext/LiteRGSS3")
set(CGSS_DIR     "${ROOT}/litecgss")

set(_violations 0)

function(_report file message)
	file(RELATIVE_PATH _rel "${ROOT}" "${file}")
	message("  ${_rel}: ${message}")
	math(EXPR _n "${_violations} + 1")
	set(_violations ${_n} PARENT_SCOPE)
endfunction()

# Fills <out> with the include targets of <file> (the text between <> or "").
function(_includes_of file out)
	file(STRINGS "${file}" _lines REGEX "^[ \t]*#[ \t]*include[ \t]*[<\"]")
	set(_result "")
	foreach(_line IN LISTS _lines)
		string(REGEX REPLACE "^[ \t]*#[ \t]*include[ \t]*[<\"]([^>\"]+)[>\"].*" "\\1" _inc "${_line}")
		list(APPEND _result "${_inc}")
	endforeach()
	set(${out} "${_result}" PARENT_SCOPE)
endfunction()

# --- 1. Bindings never name a backend --------------------------------------
file(GLOB_RECURSE _ext_sources "${EXT_DIR}/*.h" "${EXT_DIR}/*.hpp" "${EXT_DIR}/*.cpp" "${EXT_DIR}/*.c")
foreach(_f IN LISTS _ext_sources)
	_includes_of("${_f}" _incs)
	foreach(_inc IN LISTS _incs)
		if(_inc MATCHES "^(raylib|rlgl|raymath|raygui)\\.h$|^SFML/|^GLFW/")
			_report("${_f}" "includes backend header <${_inc}>; use the LiteCGSS public API instead")
		endif()
	endforeach()
	file(STRINGS "${_f}" _backend_ifs REGEX "^[ \t]*#[ \t]*(if|ifdef|ifndef|elif).*CGSS_BACKEND_")
	foreach(_line IN LISTS _backend_ifs)
		string(STRIP "${_line}" _line)
		_report("${_f}" "backend-specific code `${_line}`; move it behind a LiteCGSS public API")
	endforeach()
endforeach()

# --- 2. Bindings' CMake never widens the engine interface -------------------
file(GLOB_RECURSE _ext_cmake "${EXT_DIR}/CMakeLists.txt" "${EXT_DIR}/*.cmake")
foreach(_f IN LISTS _ext_cmake)
	file(STRINGS "${_f}" _lines REGEX "litecgss/src|LiteCGSS_SOURCE_DIR[^ ]*/src|extension_spi")
	foreach(_line IN LISTS _lines)
		string(STRIP "${_line}" _line)
		if(NOT _line MATCHES "^#")
			_report("${_f}" "reaches past LiteCGSS::engine: `${_line}`")
		endif()
	endforeach()
endforeach()

# --- 3. Engine never sees Ruby / LiteRGSS ----------------------------------
file(GLOB_RECURSE _cgss_sources
	"${CGSS_DIR}/include/*.h"
	"${CGSS_DIR}/src/*.h" "${CGSS_DIR}/src/*.cpp"
	"${CGSS_DIR}/test/*.h" "${CGSS_DIR}/test/*.cpp"
	"${CGSS_DIR}/playground/*.h" "${CGSS_DIR}/playground/*.cpp")
foreach(_f IN LISTS _cgss_sources)
	_includes_of("${_f}" _incs)
	foreach(_inc IN LISTS _incs)
		if(_inc MATCHES "^ruby\\.h$|^ruby/|RubyValue\\.h$|rbAdapter\\.h$|LiteRGSS|ext/LiteRGSS3")
			_report("${_f}" "includes <${_inc}>; LiteCGSS must not depend on Ruby or LiteRGSS")
		endif()
	endforeach()
endforeach()

# --- 4. Public headers never include private ones --------------------------
file(GLOB_RECURSE _private_headers RELATIVE "${CGSS_DIR}/src" "${CGSS_DIR}/src/*.h")
file(GLOB_RECURSE _public_headers "${CGSS_DIR}/include/*.h")
set(_spi_header "${CGSS_DIR}/include/LiteCGSS/Extension/NativeAccess.h")
foreach(_f IN LISTS _public_headers)
	if(_f STREQUAL _spi_header)
		continue()
	endif()
	_includes_of("${_f}" _incs)
	foreach(_inc IN LISTS _incs)
		if(_inc IN_LIST _private_headers)
			_report("${_f}" "public header includes private \"${_inc}\" (litecgss/src)")
		endif()
	endforeach()
endforeach()

if(_violations GREATER 0)
	message(FATAL_ERROR "Module boundary check: ${_violations} violation(s).")
endif()
message(STATUS "Module boundary check: OK")
