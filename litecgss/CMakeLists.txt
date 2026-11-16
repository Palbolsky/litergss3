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
option(LITECGSS_USE_PHYSFS "Load assets through PhysicsFS" OFF)
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
)
target_compile_features(LiteCGSS_config INTERFACE cxx_std_17)

# ---------------------------------------------------------------------------
# Backend dependencies
# ---------------------------------------------------------------------------
if(CGSS_BACKEND STREQUAL "sfml")
	find_package(SFML 2 REQUIRED COMPONENTS graphics window system)
elseif(CGSS_BACKEND STREQUAL "raylib")
	# The raylib 5.0 header ships vendored (external/raylib/raylib.h) and sets
	# the API contract; the library itself must be installed system-wide.
	# Note: raylib's 5.0 git tag still reports `PROJECT_VERSION 4.5.0`
	# (upstream bug), so no version is enforced here.
	#
	# Lookup order: an installed CMake package (raylib_DIR / system install),
	# then a prebuilt tree at RAYLIB_ROOT ({include,lib}/ — e.g. the official
	# raylib-x.y_win32_mingw-w64 zip, which ships no CMake package config).
	set(RAYLIB_ROOT "" CACHE PATH "Prebuilt raylib tree (include/ + lib/) used when no CMake package is found")
	find_package(raylib QUIET)
	if(NOT raylib_FOUND AND RAYLIB_ROOT)
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
			message(STATUS "Using prebuilt raylib from ${RAYLIB_ROOT}")
		endif()
	endif()
	if(NOT raylib_FOUND)
		message(FATAL_ERROR
			"raylib was not found. Either install it system-wide:\n"
			"  git clone https://github.com/raysan5/raylib\n"
			"  cd raylib && cmake -B build -DBUILD_SHARED_LIBS=ON && sudo cmake --install build\n"
			"or point CMake at an existing install with -Draylib_DIR=/path/to/lib/cmake/raylib,\n"
			"or at a prebuilt tree (include/ + lib/) with -DRAYLIB_ROOT=/path/to/raylib")
	endif()
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

if(LITECGSS_USE_PHYSFS)
	set(PHYSFS_BUILD_TEST OFF CACHE INTERNAL "")
	set(PHYSFS_BUILD_SHARED OFF CACHE INTERNAL "")
	add_subdirectory(${LITE_CGSS_EXTERNAL}/physfs)
	set_property(TARGET physfs-static PROPERTY POSITION_INDEPENDENT_CODE ON)
	target_include_directories(physfs-static INTERFACE ${FETCHCONTENT_BASE_DIR}/physfs-src/src)
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
	# ScrollText.{h,cpp} is a vendored copy of sf::Text — inherently SFML-
	# coupled. Non-sfml backends provide their own ScrollText under
	# Backend/Specializations/<backend>/ScrollText.{h,cpp}.
	list(FILTER LITECGSS_PRIVATE_SOURCES EXCLUDE REGEX "/src/Graphics/ScrollText\\.")
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

target_link_libraries(LiteCGSS_engine PUBLIC LiteCGSS::config skalog)
if(CGSS_BACKEND STREQUAL "sfml")
	target_link_libraries(LiteCGSS_engine PUBLIC sfml-graphics sfml-window sfml-system)
elseif(CGSS_BACKEND STREQUAL "raylib")
	target_link_libraries(LiteCGSS_engine PUBLIC raylib)
	if(WIN32)
		# Safety net: raylib's Windows build needs winmm (timeBeginPeriod).
		# Normally propagated transitively by find_package(raylib), but some
		# MinGW installs ship libraylib.a without the interface link libs.
		target_link_libraries(LiteCGSS_engine PUBLIC winmm)
	endif()
endif()
if(LITECGSS_USE_PHYSFS)
	target_link_libraries(LiteCGSS_engine PUBLIC physfs-static)
endif()

target_include_directories(LiteCGSS_engine
	PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
	PRIVATE ${LITE_CGSS_EXTERNAL}/lodepng
	PRIVATE ${LITE_CGSS_EXTERNAL}/libnsgif
)
if(CGSS_BACKEND STREQUAL "raylib")
	# Public on purpose: Backend/RaylibBackendFwd.h (public) includes
	# <raylib.h> because raylib's nested POD types cannot be forward-declared.
	target_include_directories(LiteCGSS_engine PUBLIC ${LITE_CGSS_EXTERNAL}/raylib)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(LiteCGSS_engine PRIVATE -Wall -Wextra -pedantic)
endif()

if(NOT BUILD_SHARED_LIBS)
	set_property(TARGET LiteCGSS_engine PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

if(LITECGSS_BUILD_TESTS)
	enable_testing()
	add_subdirectory(test)
endif()
if(LITECGSS_BUILD_PLAYGROUND)
	add_subdirectory(playground)
endif()
