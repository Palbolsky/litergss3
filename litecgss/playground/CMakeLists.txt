cmake_minimum_required(VERSION 3.10)
project(LiteCGSS_playground)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
	src/*.h
	src/*.cpp
)

add_definitions(-DLITE_CGSS_PLAYGROUND_DIR="${LITECGSS_ROOT_DIR}/playground")

generate_source_groups("${SOURCES}" "${LITECGSS_ROOT_DIR}\\playground\\src")

add_executable(LiteCGSS_playground ${SOURCES})

# Playground demonstrates backend-side escape hatches (`cgss::backend::native_of`),
# so it links the opt-in extension SPI (private headers + backend) rather than
# LiteCGSS::engine alone.
target_link_libraries(LiteCGSS_playground PRIVATE LiteCGSS::extension_spi)
