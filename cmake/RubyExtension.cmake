# Build a native Ruby extension with CMake (replaces mkmf / extconf.rb).
#
# Everything is queried from the target Ruby's RbConfig, like mkmf does, so
# the extension matches the interpreter that will load it. Point CMake at a
# specific interpreter with -DRUBY_EXECUTABLE=/path/to/ruby.
#
#   add_ruby_extension(<target> <sources>...)
#
# Produces <target>.<DLEXT> (no "lib" prefix) whose entry point must be
# `extern "C" void Init_<target>()`.

include_guard(GLOBAL)

# Embedded (LITERGSS_EMBEDDED): the extension is a static library linked into
# a host that embeds Ruby (litergss-everywhere's runtime, cross-compiled):
# the target Ruby cannot run here, so its header folders are given, nothing
# is queried, and <target> registers itself (Init_<target>) from the host.
if(LITERGSS_EMBEDDED)
	set(RUBY_HDR_DIR "" CACHE PATH "Embedded build: the target Ruby's header folder (RbConfig rubyhdrdir)")
	set(RUBY_ARCH_HDR_DIR "" CACHE PATH "Embedded build: the target Ruby's arch header folder (RbConfig rubyarchhdrdir, with ruby/config.h)")
	if(NOT EXISTS "${RUBY_HDR_DIR}/ruby.h" OR NOT EXISTS "${RUBY_ARCH_HDR_DIR}/ruby/config.h")
		message(FATAL_ERROR "LITERGSS_EMBEDDED: give the target Ruby's headers: -DRUBY_HDR_DIR=<dir with ruby.h> "
			"-DRUBY_ARCH_HDR_DIR=<dir with ruby/config.h> (now: '${RUBY_HDR_DIR}', '${RUBY_ARCH_HDR_DIR}')")
	endif()
	message(STATUS "Ruby (embedded): headers ${RUBY_HDR_DIR}, ${RUBY_ARCH_HDR_DIR}")

	function(add_ruby_extension target)
		add_library(${target} STATIC ${ARGN})
		set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
		target_include_directories(${target} SYSTEM PRIVATE ${RUBY_ARCH_HDR_DIR} ${RUBY_HDR_DIR})
	endfunction()
	return()
endif()

find_program(RUBY_EXECUTABLE ruby REQUIRED)

function(_ruby_rbconfig out_var key)
	execute_process(
		COMMAND ${RUBY_EXECUTABLE} -e "print RbConfig::CONFIG['${key}']"
		OUTPUT_VARIABLE _value
		RESULT_VARIABLE _result
	)
	if(NOT _result EQUAL 0)
		message(FATAL_ERROR "Unable to query RbConfig['${key}'] from ${RUBY_EXECUTABLE}")
	endif()
	set(${out_var} "${_value}" PARENT_SCOPE)
endfunction()

_ruby_rbconfig(RUBY_VERSION_STRING ruby_version)
_ruby_rbconfig(RUBY_HDR_DIR rubyhdrdir)
_ruby_rbconfig(RUBY_ARCH_HDR_DIR rubyarchhdrdir)
_ruby_rbconfig(RUBY_DLEXT DLEXT)
_ruby_rbconfig(RUBY_LIB_DIR libdir)
_ruby_rbconfig(RUBY_LIBRUBY LIBRUBY)
message(STATUS "Ruby ${RUBY_VERSION_STRING} (${RUBY_EXECUTABLE}), headers: ${RUBY_HDR_DIR}")

function(add_ruby_extension target)
	add_library(${target} MODULE ${ARGN})
	set_target_properties(${target} PROPERTIES
		PREFIX ""
		SUFFIX ".${RUBY_DLEXT}"
		POSITION_INDEPENDENT_CODE ON
	)
	target_include_directories(${target} SYSTEM PRIVATE ${RUBY_ARCH_HDR_DIR} ${RUBY_HDR_DIR})

	if(WIN32)
		# PE modules must resolve every symbol at link time: link the import lib.
		target_link_libraries(${target} PRIVATE ${RUBY_LIB_DIR}/${RUBY_LIBRUBY})
	elseif(APPLE)
		# Ruby symbols are provided by the host process at load time.
		target_link_options(${target} PRIVATE -undefined dynamic_lookup)
	endif()
	# ELF: undefined Ruby symbols are resolved by the loading interpreter.
endfunction()
