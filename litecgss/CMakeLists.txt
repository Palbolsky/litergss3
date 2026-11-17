cmake_minimum_required(VERSION 3.21)

# LiteCGSS — C++ 2D engine. Self-contained: builds standalone
# (`cmake -S litecgss -B build`) without Ruby, or as a subproject of the
# LiteRGSS3 monorepo (`add_subdirectory(litecgss)`).
#
# Consumers link `LiteCGSS::engine` and nothing else. Every setting that
# changes the public headers' meaning (backend, logs, physfs, window
# preservation) is a PUBLIC compile definition carried by the targets, so a
# consumer can never compile the headers with a different configuration than
# the library itself.
project(LiteCGSS LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(PROJECT_IS_TOP_LEVEL)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
	if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
		set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
	endif()
endif()

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
set(CGSS_BACKEND "raylib" CACHE STRING "Graphics backend for LiteCGSS (sfml, raylib, libretro)")
set_property(CACHE CGSS_BACKEND PROPERTY STRINGS sfml raylib libretro)
if(NOT CGSS_BACKEND MATCHES "^(sfml|raylib|libretro)$")
	message(FATAL_ERROR "Unknown CGSS_BACKEND '${CGSS_BACKEND}'. Must be one of: sfml, raylib, libretro.")
endif()
message(STATUS "CGSS_BACKEND=${CGSS_BACKEND}")

option(CGSS_NO_LOGS "Compile out LiteCGSS logging" OFF)
# SFML backend only: OFF for SFML ES (the SFML-opengl-es2 fork on GL ES), whose
# shaders the preprocessor rewrites to GLSL ES 1.00 on SFML ES's attributes.
option(LITECGSS_USE_FIXED_PIPELINE "SFML backend: desktop GL feeding the fixed-pipeline GLSL built-ins" ON)
option(LITECGSS_USE_PHYSFS "Load assets through PhysicsFS" OFF)
# Instruments the engine with Tracy zones. The embedder must then start
# Tracy's client before LiteCGSS runs (tracy::StartupProfiler()).
option(LITECGSS_USE_TRACY "Instrument LiteCGSS for the Tracy profiler" OFF)
# Agent channel (include/LiteCGSS/Agent/Channel.h): a local socket development
# tools use to query and drive a running game. Debug presets only; a release
# build refuses it, and it does not exist on Windows / Android.
option(LITECGSS_AGENT "Compile in the agent channel (development tooling)" OFF)
if(LITECGSS_AGENT AND (WIN32 OR ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android"))
	message(WARNING "LITECGSS_AGENT: Unix sockets only, not built on this platform")
	set(LITECGSS_AGENT OFF)
endif()
if(LITECGSS_AGENT AND (CMAKE_BUILD_TYPE STREQUAL "Release" OR CGSS_NO_LOGS))
	message(FATAL_ERROR "LITECGSS_AGENT is development tooling: it must stay OFF in a release build")
endif()
message(STATUS "LITECGSS_AGENT=${LITECGSS_AGENT}")
option(LITECGSS_BUILD_TESTS "Build the LiteCGSS unit tests" ${PROJECT_IS_TOP_LEVEL})
option(LITECGSS_BUILD_PLAYGROUND "Build the LiteCGSS playground executable" ${PROJECT_IS_TOP_LEVEL})

if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android")
	set(_cgss_window_preservation_default ON)
else()
	set(_cgss_window_preservation_default OFF)
endif()
option(CGSS_WINDOW_PRESERVATION "Keep the native window alive across DisplayWindow reloads" ${_cgss_window_preservation_default})
message(STATUS "CGSS_WINDOW_PRESERVATION=${CGSS_WINDOW_PRESERVATION}")

set(LITECGSS_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LITE_CGSS_EXTERNAL ${CMAKE_CURRENT_SOURCE_DIR}/external)

# ---------------------------------------------------------------------------
# LiteCGSS::config — the configuration contract. INTERFACE-only: it carries
# the compile definitions that the public headers depend on. Linked PUBLIC by
# the engine, and directly by white-box tests that compile engine sources.
# ---------------------------------------------------------------------------
add_library(LiteCGSS_config INTERFACE)
add_library(LiteCGSS::config ALIAS LiteCGSS_config)
string(TOUPPER "${CGSS_BACKEND}" _cgss_backend_upper)
target_compile_definitions(LiteCGSS_config INTERFACE
	CGSS_BACKEND_${_cgss_backend_upper}
	$<$<BOOL:${CGSS_NO_LOGS}>:CGSS_NO_LOGS>
	$<$<BOOL:${CGSS_WINDOW_PRESERVATION}>:CGSS_WINDOW_PRESERVATION>
	$<$<BOOL:${LITECGSS_USE_PHYSFS}>:LITECGSS_USE_PHYSFS>
	$<$<BOOL:${LITECGSS_USE_TRACY}>:LITECGSS_USE_TRACY>
)
target_compile_features(LiteCGSS_config INTERFACE cxx_std_17)
if(LITECGSS_USE_TRACY)
	# With the define that turns the zones on: whatever compiles LiteCGSS
	# sources (the engine, the unit tests) gets Tracy's headers and client.
	target_link_libraries(LiteCGSS_config INTERFACE TracyClient)
endif()

# ---------------------------------------------------------------------------
# Backend dependencies
# ---------------------------------------------------------------------------
if(CGSS_BACKEND STREQUAL "sfml")
	find_package(SFML 2 REQUIRED COMPONENTS graphics window system)
elseif(CGSS_BACKEND STREQUAL "raylib")
	# raylib >= LITECGSS_RAYLIB_MIN_VERSION. The engine compiles against the
	# headers of the raylib it links — never a copy — and checks their version.
	#
	# Where it comes from (LITECGSS_RAYLIB_SOURCE):
	#   auto    an installed CMake package (raylib_DIR / system), else a
	#           prebuilt tree at RAYLIB_ROOT ({include,lib}/, e.g. the official
	#           raylib-x.y_win32_mingw-w64 zip), else fetched (below)
	#   system  installed package or RAYLIB_ROOT only; fail otherwise
	#   fetch   always fetched: raylib LITECGSS_RAYLIB_FETCH_TAG, built static
	#           with this build (same on Linux, macOS, Windows, Android)
	set(LITECGSS_RAYLIB_MIN_VERSION 6.0)
	set(LITECGSS_RAYLIB_SOURCE "auto" CACHE STRING "Where raylib comes from: auto, system, fetch")
	set_property(CACHE LITECGSS_RAYLIB_SOURCE PROPERTY STRINGS auto system fetch)
	set(LITECGSS_RAYLIB_FETCH_TAG "6.0" CACHE STRING "raylib git tag fetched when no suitable install is found")
	set(RAYLIB_ROOT "" CACHE PATH "Prebuilt raylib tree (include/ + lib/) used when no CMake package is found")

	set(_raylib_include_dirs "")
	if(NOT LITECGSS_RAYLIB_SOURCE STREQUAL "fetch")
		# A package's own version can't be trusted (raylib 5.0's says 4.5.0):
		# the header's is checked below too.
		find_package(raylib ${LITECGSS_RAYLIB_MIN_VERSION} QUIET)
		if(raylib_FOUND)
			get_target_property(_raylib_include_dirs raylib INTERFACE_INCLUDE_DIRECTORIES)
			if(NOT _raylib_include_dirs AND raylib_INCLUDE_DIRS)
				set(_raylib_include_dirs ${raylib_INCLUDE_DIRS})
			endif()
			message(STATUS "raylib: installed package ${raylib_DIR}")
		elseif(RAYLIB_ROOT)
			find_library(RAYLIB_LIBRARY NAMES raylib PATHS ${RAYLIB_ROOT}/lib NO_DEFAULT_PATH)
			if(RAYLIB_LIBRARY AND EXISTS ${RAYLIB_ROOT}/include/raylib.h)
				add_library(raylib UNKNOWN IMPORTED)
				set_target_properties(raylib PROPERTIES
					IMPORTED_LOCATION ${RAYLIB_LIBRARY}
					INTERFACE_INCLUDE_DIRECTORIES ${RAYLIB_ROOT}/include)
				if(WIN32)
					set_property(TARGET raylib PROPERTY INTERFACE_LINK_LIBRARIES opengl32 gdi32 winmm)
				endif()
				set(raylib_FOUND TRUE)
				set(_raylib_include_dirs ${RAYLIB_ROOT}/include)
				message(STATUS "raylib: prebuilt tree ${RAYLIB_ROOT}")
			endif()
		endif()
	endif()
	if(NOT raylib_FOUND)
		if(LITECGSS_RAYLIB_SOURCE STREQUAL "system")
			message(FATAL_ERROR
				"No raylib >= ${LITECGSS_RAYLIB_MIN_VERSION} found (LITECGSS_RAYLIB_SOURCE=system). Point CMake at one with\n"
				"-Draylib_DIR=/path/to/lib/cmake/raylib or -DRAYLIB_ROOT=/path/to/raylib, or let it fetch raylib\n"
				"(-DLITECGSS_RAYLIB_SOURCE=auto).")
		endif()
		# Static, inside this build: nothing to install, and its GLFW links into
		# the engine directly (the context switch between threads needs it;
		# a *shared* raylib 6 hides it unless built with -D_GLFW_BUILD_DLL).
		include(FetchContent)
		set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
		set(CUSTOMIZE_BUILD OFF CACHE BOOL "" FORCE)
		if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android")
			set(PLATFORM "Android" CACHE STRING "" FORCE)
		endif()
		set(_cgss_shared_before_raylib ${BUILD_SHARED_LIBS})
		set(BUILD_SHARED_LIBS OFF)
		FetchContent_Declare(raylib
			GIT_REPOSITORY https://github.com/raysan5/raylib.git
			GIT_TAG ${LITECGSS_RAYLIB_FETCH_TAG}
			GIT_SHALLOW TRUE)
		FetchContent_MakeAvailable(raylib)
		set(BUILD_SHARED_LIBS ${_cgss_shared_before_raylib})
		set_target_properties(raylib PROPERTIES POSITION_INDEPENDENT_CODE ON)
		if(TARGET glfw)
			set_target_properties(glfw PROPERTIES POSITION_INDEPENDENT_CODE ON)
		endif()
		set(_raylib_include_dirs ${raylib_SOURCE_DIR}/src)
		message(STATUS "raylib: fetched ${LITECGSS_RAYLIB_FETCH_TAG}, static (${raylib_SOURCE_DIR})")
	endif()

	# The version the engine compiles against: the header's, whatever the package says.
	find_file(LITECGSS_RAYLIB_HEADER raylib.h PATHS ${_raylib_include_dirs} NO_DEFAULT_PATH NO_CACHE)
	if(NOT LITECGSS_RAYLIB_HEADER)
		message(FATAL_ERROR "raylib.h not found in ${_raylib_include_dirs}")
	endif()
	file(STRINGS ${LITECGSS_RAYLIB_HEADER} _raylib_version_line REGEX "^#define RAYLIB_VERSION[ \t]+\"")
	string(REGEX MATCH "\"([0-9.]+)" _ "${_raylib_version_line}")
	set(LITECGSS_RAYLIB_VERSION ${CMAKE_MATCH_1})
	if(LITECGSS_RAYLIB_VERSION VERSION_LESS LITECGSS_RAYLIB_MIN_VERSION)
		message(FATAL_ERROR "raylib ${LITECGSS_RAYLIB_VERSION} (${LITECGSS_RAYLIB_HEADER}) is older than the "
			"${LITECGSS_RAYLIB_MIN_VERSION} LiteCGSS needs: point raylib_DIR / RAYLIB_ROOT at a newer one, or "
			"-DLITECGSS_RAYLIB_SOURCE=fetch")
	endif()
	message(STATUS "raylib ${LITECGSS_RAYLIB_VERSION} (${LITECGSS_RAYLIB_HEADER})")
endif()

function(generate_source_groups src_files src_dir)
	foreach(_source IN ITEMS ${src_files})
		get_filename_component(_source_path "${_source}" PATH)
		string(REPLACE "/" "\\" _group_path "${_source_path}")
		string(REPLACE "/" "\\" src_dir "${src_dir}")
		string(REPLACE "${src_dir}" "" _group_path "${_group_path}")
		source_group("${_group_path}" FILES "${_source}")
	endforeach()
endfunction()

# Vendored dependencies are always static, whatever BUILD_SHARED_LIBS says.
set(_cgss_old_build_shared_libs ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)

set(SKALOG_NO_TEST ON)
add_subdirectory(external/skalog)
# skalog does not declare its own usage requirements; Common/Logger.h (public)
# includes <Logging/...>, so its include dir is part of our public interface.
target_include_directories(skalog PUBLIC ${LITE_CGSS_EXTERNAL}/skalog/src/src)

if(LITECGSS_USE_TRACY)
	# Configured as LiteCGSS2 does: every instrumented library of a process
	# (tracy_ruby...) must share this exact Tracy setup and the one shared
	# TracyClient.
	set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
	set(TRACY_ENABLE ON)
	set(TRACY_ON_DEMAND ON)
	set(TRACY_DELAYED_INIT ON)
	set(TRACY_MANUAL_LIFETIME ON)
	set(TRACY_STATIC OFF)
	# The profiled process listens for the profiler: on loopback only.
	set(TRACY_ONLY_LOCALHOST ON)
	include(FetchContent)
	FetchContent_Declare(tracy
		GIT_REPOSITORY https://github.com/wolfpld/tracy.git
		GIT_TAG v0.13.1
		GIT_SHALLOW TRUE)
	FetchContent_MakeAvailable(tracy)
endif()

if(LITECGSS_USE_PHYSFS)
	set(PHYSFS_BUILD_TEST OFF CACHE INTERNAL "")
	set(PHYSFS_BUILD_SHARED OFF CACHE INTERNAL "")
	add_subdirectory(${LITE_CGSS_EXTERNAL}/physfs)
	set_property(TARGET physfs-static PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

set(BUILD_SHARED_LIBS ${_cgss_old_build_shared_libs})

# ---------------------------------------------------------------------------
# LiteCGSS_engine target (alias LiteCGSS::engine)
#
#   include/LiteCGSS/**  -> PUBLIC headers, included as <LiteCGSS/...>
#   src/**               -> PRIVATE tree (.cpp files + backend-coupled headers
#                           such as Backend/ActiveBackend.h, SFMLBackend.h,
#                           ScrollText.h and the Internal/ helpers), included
#                           without prefix. Never on a consumer's include path.
# ---------------------------------------------------------------------------
file(GLOB_RECURSE LITECGSS_PUBLIC_HEADERS CONFIGURE_DEPENDS
	include/LiteCGSS/*.h
)
file(GLOB_RECURSE LITECGSS_PRIVATE_SOURCES CONFIGURE_DEPENDS
	src/*.h
	src/*.cpp
)
file(GLOB_RECURSE LITECGSS_VENDORED_SOURCES CONFIGURE_DEPENDS
	${LITE_CGSS_EXTERNAL}/lodepng/*.cpp
	${LITE_CGSS_EXTERNAL}/lodepng/*.h
	${LITE_CGSS_EXTERNAL}/libnsgif/*.c
	${LITE_CGSS_EXTERNAL}/libnsgif/*.h
	${LITE_CGSS_EXTERNAL}/libnsgif/*.hpp
)

# Exclude all per-backend impls from the main glob; we re-add only the
# matching backend's folder below. This is the "PERMANENT per-backend"
# bucket — TUs whose impl is fundamentally different across backends
# (Text/ScrollText, DisplayWindowSnapshot, ViewportTakeSnapshot).
list(FILTER LITECGSS_PRIVATE_SOURCES EXCLUDE REGEX "/src/Backend/Specializations/")
file(GLOB_RECURSE LITECGSS_BACKEND_IMPL_SOURCES CONFIGURE_DEPENDS
	src/Backend/Specializations/${CGSS_BACKEND}/*.cpp
	src/Backend/Specializations/${CGSS_BACKEND}/*.h
)

# Backend-gated files under src/Backend/Internal/ — their impls
# depend on a specific backend's headers (e.g. RaylibEventQueue.cpp
# includes <raylib.h>) so they must not be compiled under other backends.
# Prefixed with the backend name for the filter to work.
if(NOT CGSS_BACKEND STREQUAL "raylib")
	list(FILTER LITECGSS_PRIVATE_SOURCES EXCLUDE REGEX "/src/Backend/Internal/Raylib")
endif()
if(NOT CGSS_BACKEND STREQUAL "sfml")
	list(FILTER LITECGSS_PRIVATE_SOURCES EXCLUDE REGEX "/src/Backend/Internal/SFML")
endif()

set(LITECGSS_ALL_SOURCES
	${LITECGSS_PUBLIC_HEADERS}
	${LITECGSS_PRIVATE_SOURCES}
	${LITECGSS_BACKEND_IMPL_SOURCES}
	${LITECGSS_VENDORED_SOURCES}
)

generate_source_groups("${LITECGSS_ALL_SOURCES}" "${LITECGSS_ROOT_DIR}")

add_library(LiteCGSS_engine ${LITECGSS_ALL_SOURCES})
add_library(LiteCGSS::engine ALIAS LiteCGSS_engine)

# Only the channel's translation unit knows whether it is compiled in.
set_property(SOURCE src/Agent/Channel.cpp APPEND PROPERTY COMPILE_DEFINITIONS
	$<$<BOOL:${LITECGSS_AGENT}>:LITECGSS_AGENT_ENABLED>)
# The frame dump (LITECGSS_DUMP_FRAMES) is development tooling too.
set_property(SOURCE src/Views/FrameDump.cpp APPEND PROPERTY COMPILE_DEFINITIONS
	$<$<BOOL:${LITECGSS_AGENT}>:LITECGSS_FRAME_DUMP_ENABLED>)

# LiteCGSS_backend — the graphics backend library and its headers. Internal
# (no LiteCGSS:: alias): linked PRIVATE by the engine, so consumers get the
# backend at link time only — never its include paths. Public headers only
# forward-declare native types (Backend/<Backend>BackendFwd.h).
add_library(LiteCGSS_backend INTERFACE)
if(CGSS_BACKEND STREQUAL "sfml")
	target_link_libraries(LiteCGSS_backend INTERFACE sfml-graphics sfml-window sfml-system)
elseif(CGSS_BACKEND STREQUAL "raylib")
	# Its own headers come with the target (see the raylib lookup above).
	target_link_libraries(LiteCGSS_backend INTERFACE raylib)
	# stb_truetype: font rasterization with SFML's metrics (Internal/RaylibFontAtlas.cpp).
	target_include_directories(LiteCGSS_backend INTERFACE ${LITE_CGSS_EXTERNAL}/stb)
	if(WIN32)
		# Safety net: raylib's Windows build needs winmm (timeBeginPeriod).
		# Normally propagated transitively by find_package(raylib), but some
		# MinGW installs ship libraylib.a without the interface link libs.
		target_link_libraries(LiteCGSS_backend INTERFACE winmm)
	endif()
endif()

target_link_libraries(LiteCGSS_engine
	PUBLIC  LiteCGSS::config skalog
	PRIVATE LiteCGSS_backend
)
if(LITECGSS_USE_PHYSFS)
	target_link_libraries(LiteCGSS_engine PUBLIC physfs-static)
endif()

target_include_directories(LiteCGSS_engine
	PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
	PRIVATE ${LITE_CGSS_EXTERNAL}/lodepng
	PRIVATE ${LITE_CGSS_EXTERNAL}/libnsgif
)
if(CGSS_BACKEND STREQUAL "sfml" AND LITECGSS_USE_FIXED_PIPELINE)
	target_compile_definitions(LiteCGSS_engine PRIVATE LITECGSS_USE_FIXED_PIPELINE)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(LiteCGSS_engine PRIVATE -Wall -Wextra -pedantic)
endif()

if(NOT BUILD_SHARED_LIBS)
	set_property(TARGET LiteCGSS_engine PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

# ---------------------------------------------------------------------------
# LiteCGSS::extension_spi — opt-in escape hatch to the native backend handles
# (LiteCGSS/Extension/NativeAccess.h: video decoders, capture plugins...).
# Linking it is the explicit, reviewable act of coupling a module to the
# active backend: it exposes the private src/ tree and enables the SPI.
# ---------------------------------------------------------------------------
add_library(LiteCGSS_extension_spi INTERFACE)
add_library(LiteCGSS::extension_spi ALIAS LiteCGSS_extension_spi)
target_link_libraries(LiteCGSS_extension_spi INTERFACE LiteCGSS::engine LiteCGSS_backend)
target_include_directories(LiteCGSS_extension_spi INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(LiteCGSS_extension_spi INTERFACE CGSS_ENABLE_EXTENSION_SPI)

# Manual target to regenerate the GLSL lexer of the shader preprocessor from
# its re2c source. Not part of `all`: builds consume the committed
# glsl_lexer.cpp, so re2c (>= 3.0) is only needed when editing glsl_lexer.re:
#   cmake --build <build-dir> --target regen_glsl_lexer
find_program(RE2C_EXECUTABLE re2c)
if(RE2C_EXECUTABLE)
	add_custom_target(regen_glsl_lexer
		COMMAND ${RE2C_EXECUTABLE} -W --no-generation-date
			-o ${CMAKE_CURRENT_SOURCE_DIR}/src/Backend/Shader/preprocessor/glsl_lexer.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/src/Backend/Shader/preprocessor/glsl_lexer.re
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Regenerating glsl_lexer.cpp from glsl_lexer.re using re2c"
		VERBATIM)
endif()

if(LITECGSS_BUILD_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()
if(LITECGSS_BUILD_PLAYGROUND)
	add_subdirectory(playground)
endif()
