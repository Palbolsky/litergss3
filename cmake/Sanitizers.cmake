# Sanitizer builds (the asan preset): LITERGSS_SANITIZE=address,undefined.
#
# The engine, the extension and the C++ tests are instrumented; their
# dependencies (raylib, SFML, Ruby) are not. The Ruby interpreter that loads
# the extension isn't built with the sanitizer, so its runtime must be loaded
# before anything else: LD_PRELOAD=libasan.so. LITERGSS_SANITIZER_ENV is that
# environment, in one place:
#   - every CTest test gets it (litergss_sanitize_tests, at the end of the
#     top-level CMakeLists);
#   - the agent tooling gets it through build/<preset>/tools/build_config.rb
#     (litergss-run, litergss-scene start the game with it).
#
# The options:
#   detect_stack_use_after_return=0: the "fake stack" moves C++ locals off the
#     machine stack, where Ruby's conservative GC looks for the objects they
#     reference: an object only a local holds would be collected under it.
#   use_sigaltstack=0: Ruby installs each thread's alternate signal stack (its
#     stack overflow check); ASan, managing its own, unmapped Ruby's at thread
#     exit and aborted ("failed to deallocate ... error code: 22").
#   fast_unwind_on_malloc=0: Ruby has no frame pointers, the fast unwinder
#     stops in it; the full stacks name the frames lsan.supp matches.
#   RUBY_FREE_AT_EXIT=1 (Ruby 3.3+): Ruby frees its heap at exit instead of
#     leaving it to the system, which LeakSanitizer would report as leaks.
#   lsan.supp: what the uninstrumented parts still leave behind at exit;
#     never a LiteRGSS frame (see the file).
# A report is printed to stderr; a CTest test fails with it (non-zero exit).

set(LITERGSS_SANITIZE "" CACHE STRING
	"Sanitizers to build with (-fsanitize=<value>), e.g. address,undefined; empty: none")
set(LITERGSS_SANITIZER_ENV "")

# Gives every test of the build, in this directory and below, the sanitizer
# runtime environment. Called once all directories are added.
function(litergss_sanitize_tests dir)
	if(NOT LITERGSS_SANITIZER_ENV)
		return()
	endif()
	get_property(_tests DIRECTORY ${dir} PROPERTY TESTS)
	foreach(_test IN LISTS _tests)
		set_property(TEST ${_test} DIRECTORY ${dir} APPEND PROPERTY ENVIRONMENT ${LITERGSS_SANITIZER_ENV})
	endforeach()
	get_property(_subdirs DIRECTORY ${dir} PROPERTY SUBDIRECTORIES)
	foreach(_sub IN LISTS _subdirs)
		litergss_sanitize_tests(${_sub})
	endforeach()
endfunction()

if(NOT LITERGSS_SANITIZE)
	return()
endif()

if(LITERGSS_EMBEDDED OR CMAKE_CROSSCOMPILING)
	message(FATAL_ERROR "LITERGSS_SANITIZE: desktop builds only (a host embedding Ruby brings its own runtime)")
endif()
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
	message(FATAL_ERROR "LITERGSS_SANITIZE: needs GCC or Clang (${CMAKE_CXX_COMPILER_ID})")
endif()

add_compile_options(-fsanitize=${LITERGSS_SANITIZE} -fno-omit-frame-pointer)
add_link_options(-fsanitize=${LITERGSS_SANITIZE})

if(LITERGSS_SANITIZE MATCHES "address")
	# The runtime the compiler links against, preloaded into the interpreter
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set(_asan_runtime_name libasan.so)
	else()
		set(_asan_runtime_name libclang_rt.asan-${CMAKE_SYSTEM_PROCESSOR}.so)
	endif()
	execute_process(COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=${_asan_runtime_name}
		OUTPUT_VARIABLE LITERGSS_ASAN_RUNTIME OUTPUT_STRIP_TRAILING_WHITESPACE)
	if(NOT IS_ABSOLUTE "${LITERGSS_ASAN_RUNTIME}" OR NOT EXISTS "${LITERGSS_ASAN_RUNTIME}")
		message(FATAL_ERROR "LITERGSS_SANITIZE: ${_asan_runtime_name} not found (${CMAKE_CXX_COMPILER} -print-file-name)")
	endif()
	list(APPEND LITERGSS_SANITIZER_ENV
		"LD_PRELOAD=${LITERGSS_ASAN_RUNTIME}"
		"ASAN_OPTIONS=detect_leaks=1:detect_stack_use_after_return=0:use_sigaltstack=0:fast_unwind_on_malloc=0:strict_init_order=1"
		"RUBY_FREE_AT_EXIT=1"
		"LSAN_OPTIONS=suppressions=${PROJECT_SOURCE_DIR}/tools/sanitizers/lsan.supp:print_suppressions=0")
endif()
if(LITERGSS_SANITIZE MATCHES "undefined")
	list(APPEND LITERGSS_SANITIZER_ENV "UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1")
endif()
message(STATUS "Sanitizers: ${LITERGSS_SANITIZE} (runtime environment: ${LITERGSS_SANITIZER_ENV})")
