cmake_minimum_required(VERSION 3.21)

# LiteRGSS3 monorepo.
#
#   litecgss/       C++ engine. Knows nothing about Ruby nor LiteRGSS; builds
#                   standalone with `cmake -S litecgss -B <dir>`.
#   ext/LiteRGSS3/  Ruby bindings. Only consumer of the engine, through the
#                   LiteCGSS::engine target (public include/ tree only).
#
# Prefer the presets: `cmake --preset debug && cmake --build --preset debug`
# (or `rake compile`, which proxies to them).
project(LiteRGSS3 VERSION 3.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(LITERGSS_BUILD_RUBY_EXT "Build the LiteRGSS Ruby extension (requires Ruby)" ON)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Historical location of a prebuilt raylib (see README).
if(EXISTS ${PROJECT_SOURCE_DIR}/vendor/raylib/include/raylib.h)
	set(RAYLIB_ROOT ${PROJECT_SOURCE_DIR}/vendor/raylib CACHE PATH "")
endif()

enable_testing()

# Module boundaries (cmake/CheckBoundaries.cmake) are part of the build: a
# violation fails `cmake --build`, not only a lint run.
option(LITERGSS_CHECK_BOUNDARIES "Fail the build on LiteCGSS/LiteRGSS boundary violations" ON)
if(LITERGSS_CHECK_BOUNDARIES)
	add_custom_target(check_boundaries ALL
		COMMAND ${CMAKE_COMMAND} -P ${PROJECT_SOURCE_DIR}/cmake/CheckBoundaries.cmake
		COMMENT "Checking LiteCGSS/LiteRGSS module boundaries"
		VERBATIM)
endif()

add_subdirectory(litecgss)

if(LITERGSS_BUILD_RUBY_EXT)
	add_subdirectory(ext/LiteRGSS3)
endif()
