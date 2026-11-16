# LiteRGSS — Ruby bindings over LiteCGSS.
#
# Boundary: the only way into the engine is LiteCGSS::engine, which exposes
# the public litecgss/include/ tree (as <LiteCGSS/...>) plus the
# configuration defines it was compiled with. Never add litecgss/src (or any
# other engine path) to this target's include directories.

include(RubyExtension)

file(GLOB_RECURSE LITERGSS_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

add_ruby_extension(LiteRGSS ${LITERGSS_SOURCES})

# The Ruby side (tests/, start.rb, PSDK dev setups) loads build/lib/LiteRGSS.
set(LITERGSS_EXT_OUTPUT_DIR ${PROJECT_SOURCE_DIR}/build/lib CACHE PATH
	"Where the LiteRGSS extension is written")
set_target_properties(LiteRGSS PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY ${LITERGSS_EXT_OUTPUT_DIR}
	RUNTIME_OUTPUT_DIRECTORY ${LITERGSS_EXT_OUTPUT_DIR}
)

# Bindings include their own headers by bare name ("Color.h") from any
# subfolder: every subfolder is an include root.
file(GLOB _litergss_children LIST_DIRECTORIES true ${CMAKE_CURRENT_SOURCE_DIR}/*)
set(_litergss_include_dirs ${CMAKE_CURRENT_SOURCE_DIR})
foreach(_child IN LISTS _litergss_children)
	if(IS_DIRECTORY ${_child})
		list(APPEND _litergss_include_dirs ${_child})
	endif()
endforeach()
target_include_directories(LiteRGSS PRIVATE ${_litergss_include_dirs})

target_link_libraries(LiteRGSS PRIVATE LiteCGSS::engine)

# TEMPORARY: a few bindings still reach the engine's private backend layer
# (Backend/ActiveBackend.h). Removed once LiteCGSS exposes the public APIs
# they need.
target_include_directories(LiteRGSS PRIVATE ${LiteCGSS_SOURCE_DIR}/src)

# Ruby test suite (non-interactive files only: *_live.rb open a real window
# and wait for user input). One CTest entry per file.
file(GLOB _litergss_ruby_tests ${PROJECT_SOURCE_DIR}/tests/test_*.rb)
list(FILTER _litergss_ruby_tests EXCLUDE REGEX "_live\\.rb$")
foreach(_test IN LISTS _litergss_ruby_tests)
	get_filename_component(_name ${_test} NAME_WE)
	add_test(NAME LiteRGSS.${_name}
		COMMAND ${RUBY_EXECUTABLE} -Itests ${_test}
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
endforeach()
