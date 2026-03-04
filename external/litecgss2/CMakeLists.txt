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

project(LiteCGSS2_engine CXX)

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
endif()

set(LITECGSS_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LITE_CGSS_EXTERNAL ${CMAKE_CURRENT_SOURCE_DIR}/external)

#find_package(SFML 2 REQUIRED COMPONENTS graphics window system)

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

add_subdirectory(src)
#if(NOT LITECGSS_NO_TEST)
#  add_subdirectory(test)
#endif()
#add_subdirectory(playground)