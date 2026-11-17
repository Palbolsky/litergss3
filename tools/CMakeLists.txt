# Agent tooling, staged per build: build/<preset>/bin/{litergss-agent,envs,litergss-run,litergss-scene,litergss-profile}.
#
# Each CLI is a few lines configured from <name>.in: it loads this build's
# facts (build/<preset>/tools/build_config.rb — Ruby, extension, backend,
# build type) and the shared code in tools/support/. So a tool never reads
# CMakeCache.txt, and every preset has its own tools side by side.
# tools/psdk-demo.sh reads the same facts from build_config.sh.
#
# Usage and the fences each tool keeps: AGENTS.md ("How to reach the running game").

get_filename_component(LITERGSS_PRESET_NAME ${CMAKE_BINARY_DIR} NAME)
if(NOT CMAKE_BUILD_TYPE)
	set(LITERGSS_TOOLS_BUILD_TYPE "")
else()
	set(LITERGSS_TOOLS_BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif()
if(LITECGSS_AGENT)
	set(LITERGSS_TOOLS_AGENT true)
else()
	set(LITERGSS_TOOLS_AGENT false)
endif()
if(NOT LITERGSS_EXT_NAME)
	message(FATAL_ERROR "tools/: LITERGSS_EXT_NAME unset (ext/LiteRGSS3 must be added first)")
endif()

# The LiteRGSS2 reference (litergss-scene's oracle): opt-in, never in `all`.
#   cmake --build --preset debug --target litergss2-reference
set(LITERGSS2_REFERENCE_DIR ${PROJECT_SOURCE_DIR}/../litergss2 CACHE PATH "LiteRGSS2 checkout built as litergss-scene's reference")
set(LITERGSS2_REFERENCE_SFML ${PROJECT_SOURCE_DIR}/vendor/sfml CACHE PATH "SFML 2.x install prefix the LiteRGSS2 reference links")
set(LITERGSS2_REFERENCE_OUTPUT ${CMAKE_BINARY_DIR}/reference/litergss2)
add_custom_target(litergss2-reference
	COMMAND ${CMAKE_COMMAND} -DSOURCE=${LITERGSS2_REFERENCE_DIR} -DOUTPUT=${LITERGSS2_REFERENCE_OUTPUT}
		-DRUBY=${RUBY_EXECUTABLE} -DSFML_ROOT=${LITERGSS2_REFERENCE_SFML}
		-P ${PROJECT_SOURCE_DIR}/cmake/BuildLiteRGSS2Reference.cmake
	COMMENT "Building the LiteRGSS2 reference from ${LITERGSS2_REFERENCE_DIR} (a copy)"
	USES_TERMINAL VERBATIM)

# Tracy's command-line capture and CSV export (litergss-profile): provided
# programs, found on PATH or in the development prefix (vendor/deps/bin, built
# by tools/deps/build-deps.sh), never built here.
if(LITECGSS_USE_TRACY)
	find_program(LITERGSS_TRACY_CAPTURE NAMES tracy-capture)
	find_program(LITERGSS_TRACY_CSVEXPORT NAMES tracy-csvexport)
	set(LITERGSS_TOOLS_TRACY true)
else()
	set(LITERGSS_TOOLS_TRACY false)
endif()

configure_file(build_config.rb.in ${CMAKE_BINARY_DIR}/tools/build_config.rb @ONLY)
configure_file(build_config.sh.in ${CMAKE_BINARY_DIR}/tools/build_config.sh @ONLY)

foreach(_tool IN ITEMS litergss-agent envs litergss-run litergss-scene litergss-profile)
	configure_file(${_tool}.in ${CMAKE_BINARY_DIR}/bin/${_tool} @ONLY
		FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endforeach()

# The tooling end to end on a fixture game (tests/agent/): envs up,
# litergss-run, state / shot / eval, envs stop, SIGKILL fallback.
if(LITECGSS_AGENT AND NOT LITECGSS_USE_TRACY)
	add_test(NAME LiteRGSS.agent_tooling
		COMMAND ${RUBY_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/agent/test_agent_tooling.rb ${CMAKE_BINARY_DIR}/bin
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
	set_tests_properties(LiteRGSS.agent_tooling PROPERTIES TIMEOUT 120)
endif()
