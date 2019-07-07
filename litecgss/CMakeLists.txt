cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

set(LITE_CGSS_DIR ${CMAKE_BINARY_DIR})
set(LITE_CGSS_EXTERNAL ${CMAKE_BINARY_DIR}/external)
set(CMAKE_MODULE_PATH cmake)

project(LiteCGSS_engine)

find_package(SFML 2 REQUIRED COMPONENTS network audio graphics window system)

add_subdirectory(src)
#add_subdirectory(test)
add_subdirectory(playground)