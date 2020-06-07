cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(LITECGSS_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LITE_CGSS_EXTERNAL ${CMAKE_CURRENT_SOURCE_DIR}/external)

project(LiteCGSS_engine)

find_package(SFML 2 REQUIRED COMPONENTS graphics window system)

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
add_subdirectory(external/skalog)
set(BUILD_SHARED_LIBS ${OLD_BUILD_SHARED_LIBS})

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(playground)