# Module boundary check between LiteCGSS, its graphics backends and LiteRGSS.
#
#   cmake -P cmake/CheckBoundaries.cmake      (or `rake lint`; also run by every build)
#
# The build already makes most violations impossible: litecgss/src is not on
# LiteRGSS' include path, backend headers are not on any consumer's include
# path, and public headers only forward-declare native backend types. This
# script guards what include paths cannot:
#
#   1. ext/LiteRGSS3   never names a backend: no backend header, no
#                      LiteCGSS/Backend/* header, no CGSS_BACKEND_* branch,
#                      no backend descriptor / sf:: identifier.
#   2. ext/LiteRGSS3   CMake never widens the engine interface (no
#                      litecgss/src path, no LiteCGSS::extension_spi).
#   3. litecgss        never sees Ruby nor LiteRGSS.
#   4. litecgss/include (public) never includes a private src/ header
#                      (except Extension/NativeAccess.h, the opt-in SPI).
#   5. litecgss outside Backend/ folders is backend-neutral: same
#                      restrictions as 1, plus no backend-specific Backend/
#                      header (only ActiveBackend[Fwd].h / BackendCommon.h /
#                      BackendConcept.h). Backend code lives in
#                      include/LiteCGSS/Backend/ and src/Backend/ only.
#
# Identifier checks ignore `//` comments and comment-only lines.
# Exit code is non-zero when a violation is found.

cmake_minimum_required(VERSION 3.21)

get_filename_component(ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(EXT_DIR  "${ROOT}/ext/LiteRGSS3")
set(CGSS_DIR "${ROOT}/litecgss")

set(BACKEND_HEADERS_RE "^(raylib|rlgl|raymath|raygui)\\.h$|^SFML/|^GLFW/")
# Backend-specific identifiers: sf:: namespace, backend descriptors, raylib's
# global native structs (`::Texture`, but not `cgss::Texture`).
set(BACKEND_IDENTIFIERS_RE "(^|[^A-Za-z0-9_])sf::|(SFMLBackend|RaylibBackend|LibretroBackend)|(^|[^A-Za-z0-9_:>])::(Texture|Image|Font|Shader|RenderTexture|Mesh|Camera2D|Vector2)([^A-Za-z0-9_]|$)")

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

# Fills <out> with the lines of <file> matching <regex>, comments and string
# literals removed (so prose mentioning a backend is not a violation).
function(_code_lines_matching file regex out)
	file(READ "${file}" _content)
	# `;` is CMake's list separator: neutralize it before splitting lines.
	string(REPLACE ";" "," _content "${_content}")
	string(REGEX REPLACE "/\\*([^*]|\\*+[^*/])*\\*+/" "" _content "${_content}")
	string(REGEX REPLACE "//[^\n]*" "" _content "${_content}")
	string(REGEX REPLACE "\"([^\"\\\\\n]|\\\\.)*\"" "\"\"" _content "${_content}")
	string(REPLACE "\n" ";" _lines "${_content}")
	set(_result "")
	foreach(_line IN LISTS _lines)
		if(_line MATCHES "${regex}")
			string(STRIP "${_line}" _line)
			list(APPEND _result "${_line}")
		endif()
	endforeach()
	set(${out} "${_result}" PARENT_SCOPE)
endfunction()

# Checks shared by the bindings and the backend-neutral engine code.
# <extra_include_re>: additional forbidden include targets.
function(_check_backend_neutral file extra_include_re hint)
	_includes_of("${file}" _incs)
	foreach(_inc IN LISTS _incs)
		if(_inc MATCHES "${BACKEND_HEADERS_RE}" OR _inc MATCHES "${extra_include_re}")
			_report("${file}" "includes <${_inc}>; ${hint}")
		endif()
	endforeach()
	file(STRINGS "${file}" _backend_ifs REGEX "^[ \t]*#[ \t]*(if|ifdef|ifndef|elif).*CGSS_BACKEND_")
	foreach(_line IN LISTS _backend_ifs)
		string(STRIP "${_line}" _line)
		_report("${file}" "backend-specific branch `${_line}`; ${hint}")
	endforeach()
	_code_lines_matching("${file}" "${BACKEND_IDENTIFIERS_RE}" _hits)
	foreach(_code IN LISTS _hits)
		_report("${file}" "names a backend: `${_code}`; ${hint}")
	endforeach()
	set(_violations ${_violations} PARENT_SCOPE)
endfunction()

# --- 1. Bindings never name a backend --------------------------------------
file(GLOB_RECURSE _ext_sources "${EXT_DIR}/*.h" "${EXT_DIR}/*.hpp" "${EXT_DIR}/*.cpp" "${EXT_DIR}/*.c")
foreach(_f IN LISTS _ext_sources)
	_check_backend_neutral("${_f}" "^LiteCGSS/Backend/"
		"use a backend-neutral LiteCGSS public API instead")
	_code_lines_matching("${_f}" "(^|[^A-Za-z0-9_])ActiveBackend([^A-Za-z0-9_]|$)" _hits)
	foreach(_code IN LISTS _hits)
		_report("${_f}" "reaches the backend descriptor: `${_code}`; use a LiteCGSS public API instead")
	endforeach()
endforeach()

# --- 2. Bindings' CMake never widens the engine interface -------------------
file(GLOB_RECURSE _ext_cmake "${EXT_DIR}/CMakeLists.txt" "${EXT_DIR}/*.cmake")
foreach(_f IN LISTS _ext_cmake)
	file(STRINGS "${_f}" _lines REGEX "litecgss/src|LiteCGSS_SOURCE_DIR[^ ]*/src|extension_spi|LiteCGSS_backend")
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

# --- 5. Engine code outside Backend/ is backend-neutral ---------------------
# The playground is excluded: it demonstrates the extension SPI (native
# handles) on purpose.
file(GLOB_RECURSE _neutral_sources
	"${CGSS_DIR}/include/*.h"
	"${CGSS_DIR}/src/*.h" "${CGSS_DIR}/src/*.cpp"
	"${CGSS_DIR}/test/*.h" "${CGSS_DIR}/test/*.cpp")
list(FILTER _neutral_sources EXCLUDE REGEX "^${CGSS_DIR}/(include/LiteCGSS|src)/Backend/")
foreach(_f IN LISTS _neutral_sources)
	_check_backend_neutral("${_f}"
		"Backend/(SFML|Raylib|Libretro)|Backend/(Internal|Specializations)/"
		"move it under Backend/ (an Ops entry or a Specializations/<backend>/ file)")
endforeach()

if(_violations GREATER 0)
	message(FATAL_ERROR "Module boundary check: ${_violations} violation(s).")
endif()
message(STATUS "Module boundary check: OK")
