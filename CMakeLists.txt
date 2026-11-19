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
# Embedded: the extension is a static library (libLiteRGSS.a, Init_LiteRGSS)
# for a host embedding Ruby, typically cross-compiled (litergss-everywhere,
# the android-* presets): the target Ruby's headers are given (RUBY_HDR_DIR,
# RUBY_ARCH_HDR_DIR), no Ruby runs, no agent tooling. `cmake --install`
# installs the engine, the extension and the public headers.
option(LITERGSS_EMBEDDED "Build LiteRGSS as a static library for a host embedding Ruby" OFF)
include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# The development prefix: dependencies built by tools/deps/build-deps.sh
# (raylib, PhysicsFS, GoogleTest, Tracy). Found like any other install;
# LiteRGSS never downloads its dependencies itself.
if(EXISTS ${PROJECT_SOURCE_DIR}/vendor/deps AND NOT CMAKE_CROSSCOMPILING)
	list(PREPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/vendor/deps)
endif()
# Its Android counterpart (build-deps.sh --android <ABI>). A cross toolchain
# (the NDK's) searches packages only under CMAKE_FIND_ROOT_PATH: the prefix
# is added there too. litergss-everywhere passes its own staging prefix.
if(ANDROID AND ANDROID_ABI AND EXISTS ${PROJECT_SOURCE_DIR}/vendor/deps-android-${ANDROID_ABI})
	list(PREPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/vendor/deps-android-${ANDROID_ABI})
	list(APPEND CMAKE_FIND_ROOT_PATH ${PROJECT_SOURCE_DIR}/vendor/deps-android-${ANDROID_ABI})
endif()

# Historical location of a prebuilt raylib (see README).
if(EXISTS ${PROJECT_SOURCE_DIR}/vendor/raylib/include/raylib.h)
	set(RAYLIB_ROOT ${PROJECT_SOURCE_DIR}/vendor/raylib CACHE PATH "")
endif()
# Likewise an SFML 2.6 install for the SFML backend (cmake --install <sfml-build>
# --prefix vendor/sfml), unless SFML_DIR names another one.
if(EXISTS ${PROJECT_SOURCE_DIR}/vendor/sfml/lib/cmake/SFML/SFMLConfig.cmake)
	set(SFML_DIR ${PROJECT_SOURCE_DIR}/vendor/sfml/lib/cmake/SFML CACHE PATH "SFML 2.6 CMake package")
endif()

enable_testing()

# Sanitizer builds (the asan preset): LITERGSS_SANITIZE=address,undefined
include(Sanitizers)

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
	# Agent tooling (litergss-agent, envs, litergss-run), staged into
	# build/<preset>/bin with this build's Ruby, extension and backend.
	if(NOT LITERGSS_EMBEDDED)
		add_subdirectory(tools)
	endif()
endif()

# The sanitizer runtime environment on every test (no-op without LITERGSS_SANITIZE)
litergss_sanitize_tests(${PROJECT_SOURCE_DIR})
