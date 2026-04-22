cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CGSS_NO_LOGS)
  set(CMAKE_CXX_FLAGS "-DCGSS_NO_LOGS")
endif()

project(LiteCGSS_engine CXX C)

if(NOT DEFINED CGSS_WINDOW_PRESERVATION)
  if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(CGSS_WINDOW_PRESERVATION ON)
  else()
    set(CGSS_WINDOW_PRESERVATION OFF)
  endif()
endif()
message(STATUS "CGSS_WINDOW_PRESERVATION=${CGSS_WINDOW_PRESERVATION} (ANDROID=${ANDROID}, CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME})")
if(CGSS_WINDOW_PRESERVATION)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCGSS_WINDOW_PRESERVATION")
endif()

set(CGSS_BACKEND "sfml" CACHE STRING "Graphics backend for LiteCGSS (sfml, raylib, libretro)")
set_property(CACHE CGSS_BACKEND PROPERTY STRINGS sfml raylib libretro)
if(CGSS_BACKEND STREQUAL "sfml")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCGSS_BACKEND_SFML")
elseif(CGSS_BACKEND STREQUAL "raylib")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCGSS_BACKEND_RAYLIB")
elseif(CGSS_BACKEND STREQUAL "libretro")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCGSS_BACKEND_LIBRETRO")
else()
  message(FATAL_ERROR "Unknown CGSS_BACKEND '${CGSS_BACKEND}'. Must be one of: sfml, raylib, libretro.")
endif()
message(STATUS "CGSS_BACKEND=${CGSS_BACKEND}")

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

set(LITECGSS_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LITE_CGSS_EXTERNAL ${CMAKE_CURRENT_SOURCE_DIR}/external)

if(CGSS_BACKEND STREQUAL "sfml")
  find_package(SFML 2 REQUIRED COMPONENTS graphics window system)
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

set(OLD_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS False)

set(SKALOG_NO_TEST True)
add_subdirectory(external/skalog)

if (LITECGSS_USE_PHYSFS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLITECGSS_USE_PHYSFS")
  set(PHYSFS_BUILD_TEST OFF CACHE INTERNAL "")
  set(PHYSFS_BUILD_SHARED OFF CACHE INTERNAL "")
  add_subdirectory(${LITE_CGSS_EXTERNAL}/physfs)
endif()

set(BUILD_SHARED_LIBS ${OLD_BUILD_SHARED_LIBS})

# ---------------------------------------------------------------------------
# LiteCGSS_engine target
#
#   include/LiteCGSS/**  -> PUBLIC headers propagated to consumers
#   src/LiteCGSS/**      -> PRIVATE tree (.cpp files + backend-coupled headers
#                           such as ActiveBackend.h, SFMLBackend.h, ScrollText.h
#                           and the Internal/ Sf*Conv.h helpers). Not reachable
#                           from consumer TUs.
# ---------------------------------------------------------------------------

file(GLOB_RECURSE LITECGSS_PUBLIC_HEADERS CONFIGURE_DEPENDS
	include/LiteCGSS/*.h
)
file(GLOB_RECURSE LITECGSS_PRIVATE_SOURCES CONFIGURE_DEPENDS
	src/LiteCGSS/*.h
	src/LiteCGSS/*.cpp
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
list(FILTER LITECGSS_PRIVATE_SOURCES EXCLUDE REGEX "/LiteCGSS/Backend/Specializations/")
file(GLOB_RECURSE LITECGSS_BACKEND_IMPL_SOURCES CONFIGURE_DEPENDS
	src/LiteCGSS/Backend/Specializations/${CGSS_BACKEND}/*.cpp
	src/LiteCGSS/Backend/Specializations/${CGSS_BACKEND}/*.h
)

set(LITECGSS_ALL_SOURCES
	${LITECGSS_PUBLIC_HEADERS}
	${LITECGSS_PRIVATE_SOURCES}
	${LITECGSS_BACKEND_IMPL_SOURCES}
	${LITECGSS_VENDORED_SOURCES}
)

generate_source_groups("${LITECGSS_ALL_SOURCES}" "${LITECGSS_ROOT_DIR}")

add_library(LiteCGSS_engine ${LITECGSS_ALL_SOURCES})

set(TARGET_LIBS skalog)
if(CGSS_BACKEND STREQUAL "sfml")
	set(TARGET_LIBS ${TARGET_LIBS} sfml-graphics sfml-window sfml-system)
endif()
if(LITECGSS_USE_PHYSFS)
	set(TARGET_LIBS ${TARGET_LIBS} physfs-static)
	set_property(TARGET physfs-static PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

target_link_libraries(LiteCGSS_engine PUBLIC ${TARGET_LIBS})

target_include_directories(LiteCGSS_engine
	PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
	PUBLIC  ${LITE_CGSS_EXTERNAL}/lodepng
	PUBLIC  ${LITE_CGSS_EXTERNAL}/libnsgif
	PUBLIC  ${LITE_CGSS_EXTERNAL}/skalog/src/src
)
if(LITECGSS_USE_PHYSFS)
	target_include_directories(LiteCGSS_engine
		PUBLIC ${FETCHCONTENT_BASE_DIR}/physfs-src/src)
endif()
if(CGSS_BACKEND STREQUAL "raylib")
	# Header-only use of raylib for PR M scope (b): types only, no link.
	target_include_directories(LiteCGSS_engine PUBLIC ${LITE_CGSS_EXTERNAL}/raylib)
endif()

if(NOT BUILD_SHARED_LIBS)
	set_property(TARGET LiteCGSS_engine PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

if(NOT LITECGSS_NO_TEST)
	add_subdirectory(test)
endif()
add_subdirectory(playground)