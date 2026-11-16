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

# A Tracy build needs the host to start the profiler before any LiteRGSS
# call, so it is a different extension (LiteRGSS_tracy, as in LiteRGSS2),
# loaded with the shared TracyClient written next to it.
if(LITECGSS_USE_TRACY)
	set(LITERGSS_EXT_NAME LiteRGSS_tracy)
	set_target_properties(TracyClient PROPERTIES
		LIBRARY_OUTPUT_DIRECTORY ${LITERGSS_EXT_OUTPUT_DIR}
		RUNTIME_OUTPUT_DIRECTORY ${LITERGSS_EXT_OUTPUT_DIR})
	if(APPLE)
		set_target_properties(LiteRGSS PROPERTIES BUILD_RPATH "@loader_path" INSTALL_RPATH "@loader_path")
	elseif(UNIX)
		set_target_properties(LiteRGSS PROPERTIES BUILD_RPATH "$ORIGIN" INSTALL_RPATH "$ORIGIN")
	endif()
else()
	set(LITERGSS_EXT_NAME LiteRGSS)
endif()
set_target_properties(LiteRGSS PROPERTIES OUTPUT_NAME ${LITERGSS_EXT_NAME})
target_compile_definitions(LiteRGSS PRIVATE LITERGSS_INIT_FUNCTION=Init_${LITERGSS_EXT_NAME})

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

# Ruby test suite (non-interactive files only: *_live.rb open a real window
# and wait for user input). One CTest entry per file.
# Not in a Tracy build: its extension needs a host starting the profiler
# first (tracy_ruby), which the suite has no way to do.
file(GLOB _litergss_ruby_tests ${PROJECT_SOURCE_DIR}/tests/test_*.rb)
if(LITECGSS_USE_TRACY)
	set(_litergss_ruby_tests "")
endif()
list(FILTER _litergss_ruby_tests EXCLUDE REGEX "_live\\.rb$")
foreach(_test IN LISTS _litergss_ruby_tests)
	get_filename_component(_name ${_test} NAME_WE)
	add_test(NAME LiteRGSS.${_name}
		COMMAND ${RUBY_EXECUTABLE} -Itests ${_test}
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
	# The extension this build produced (tests/litergss.rb loads it), its
	# windows hidden: the suite doesn't open windows on the desktop
	set_tests_properties(LiteRGSS.${_name} PROPERTIES
		ENVIRONMENT "LITERGSS_LIB=${LITERGSS_EXT_OUTPUT_DIR}/LiteRGSS;LITERGSS_HIDDEN_WINDOW=1")
endforeach()
