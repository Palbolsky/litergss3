cmake_minimum_required(VERSION 3.10)
project(LiteCGSS_playground)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
	src/*.h
	src/*.cpp
)

add_definitions(-DLITE_CGSS_PLAYGROUND_DIR="${LITECGSS_ROOT_DIR}/playground")

generate_source_groups("${SOURCES}" "${LITECGSS_ROOT_DIR}\\playground\\src")

add_executable(LiteCGSS_playground ${SOURCES})

target_link_libraries(LiteCGSS_playground PRIVATE LiteCGSS_engine)
if(CGSS_BACKEND STREQUAL "sfml")
	target_link_libraries(LiteCGSS_playground PRIVATE sfml-graphics sfml-window sfml-system)
endif()

# Playground is allowed to reach LiteCGSS's PRIVATE headers (it demonstrates
# `cgss::backend::native_of` and other backend-side escape hatches). External
# consumers (e.g. the LiteRGSS2 Ruby binding) link against LiteCGSS_engine and
# only see the PUBLIC include/ tree.
target_include_directories(LiteCGSS_playground PRIVATE ${LITECGSS_ROOT_DIR}/src)
