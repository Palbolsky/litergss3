# Builds the LiteRGSS2 reference extension (the `litergss2-reference` target):
# the visual oracle of build/<preset>/bin/litergss-scene.
#
#   cmake -DSOURCE=<litergss2 checkout> -DOUTPUT=<dir> -DRUBY=<ruby> -DSFML_ROOT=<sfml 2.6 prefix> \
#         -P cmake/BuildLiteRGSS2Reference.cmake
#
# The reference is the local LiteRGSS2 working tree (local changes included),
# never a release; its LiteCGSS is the commit LiteRGSS2 records (see 1b). It is built from a copy in
# OUTPUT/src — the checkout itself is never written to — the way LiteRGSS2's
# Rakefile does it: LiteCGSS (static, SFML 2) in-source, then the extension
# with mkmf (extconf.rb + make; no rake-compiler needed). Result:
# OUTPUT/lib/LiteRGSS.so and OUTPUT/reference.txt (what was built).

foreach(_var SOURCE OUTPUT RUBY SFML_ROOT)
	if(NOT ${_var})
		message(FATAL_ERROR "BuildLiteRGSS2Reference: ${_var} is not set")
	endif()
endforeach()
if(NOT EXISTS "${SOURCE}/ext/LiteRGSS/extconf.rb")
	message(FATAL_ERROR "No LiteRGSS2 checkout at ${SOURCE} (set LITERGSS2_REFERENCE_DIR)")
endif()
if(NOT EXISTS "${SFML_ROOT}/lib/cmake/SFML/SFMLConfig.cmake")
	message(FATAL_ERROR "LiteRGSS2 needs SFML 2.x: none at ${SFML_ROOT} (see README, vendor/sfml)")
endif()
find_program(RSYNC rsync REQUIRED)

function(run)
	execute_process(COMMAND ${ARGN} RESULT_VARIABLE _status)
	if(NOT _status EQUAL 0)
		list(JOIN ARGN " " _command)
		message(FATAL_ERROR "litergss2-reference: failed (${_status}): ${_command}")
	endif()
endfunction()

set(_src "${OUTPUT}/src")
set(_cgss "${_src}/external/litecgss")

# 1. The working tree, without version control nor the checkout's own build output.
file(MAKE_DIRECTORY "${_src}")
run(${RSYNC} -a --delete
	--exclude=.git --exclude=/tmp/ --exclude=/lib/ --exclude=*.log
	--exclude=/external/litecgss/CMakeCache.txt --exclude=/external/litecgss/CMakeFiles/
	--exclude=/external/litecgss/Makefile --exclude=/external/litecgss/cmake_install.cmake
	--exclude=/external/litecgss/bin/ --exclude=/external/litecgss/lib/
	--exclude=/external/litecgss/build*/ --exclude=/external/litecgss/_deps/*-build/
	--exclude=/external/litecgss/_deps/*-subbuild/
	"${SOURCE}/" "${_src}/")

# 1b. LiteCGSS at the commit LiteRGSS2 records. A checkout whose submodule
# was not updated (`git submodule status` shows `+`) pairs the extension
# with a LiteCGSS it does not build against; the recorded one is the
# repository's own state. Exported read-only (git archive), with its own
# submodules at their recorded commits.
execute_process(COMMAND git -C "${SOURCE}" ls-tree HEAD external/litecgss
	OUTPUT_VARIABLE _tree OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX MATCH "[0-9a-f]+\texternal/litecgss$" _recorded "${_tree}")
string(REGEX REPLACE "\t.*" "" _recorded "${_recorded}")
execute_process(COMMAND git -C "${SOURCE}/external/litecgss" rev-parse HEAD
	OUTPUT_VARIABLE _checked_out OUTPUT_STRIP_TRAILING_WHITESPACE)
set(_cgss_note "checkout")
if(_recorded AND NOT _recorded STREQUAL _checked_out)
	set(_export "${OUTPUT}/litecgss-recorded")
	file(REMOVE_RECURSE "${_export}")
	file(MAKE_DIRECTORY "${_export}")
	run(sh -c "git -C '${SOURCE}/external/litecgss' archive --format=tar ${_recorded} | tar -x -C '${_export}'")
	run(${RSYNC} -a --delete --exclude=/CMakeCache.txt --exclude=/CMakeFiles/
		--exclude=/Makefile --exclude=/cmake_install.cmake --exclude=/bin/ --exclude=/lib/ --exclude=/_deps/
		"${_export}/" "${_cgss}/")
	# Its own submodules (skalog, tracy...) at the commits it records too: from
	# the local module repository when it has the commit, else fetched (that
	# commit only) from the URL in its .gitmodules.
	execute_process(COMMAND git -C "${SOURCE}/external/litecgss" ls-tree -r ${_recorded}
		OUTPUT_VARIABLE _entries OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND git -C "${SOURCE}/external/litecgss" show ${_recorded}:.gitmodules
		OUTPUT_VARIABLE _gitmodules)
	file(WRITE "${OUTPUT}/litecgss.gitmodules" "${_gitmodules}")
	string(REPLACE "\n" ";" _entries "${_entries}")
	foreach(_entry IN LISTS _entries)
		if(NOT _entry MATCHES "^160000 commit ([0-9a-f]+)\t(.+)$")
			continue()
		endif()
		set(_commit ${CMAKE_MATCH_1})
		set(_path ${CMAKE_MATCH_2})
		set(_module_git "${SOURCE}/.git/modules/external/litecgss/modules/${_path}")
		execute_process(COMMAND git --git-dir=${_module_git} cat-file -t ${_commit}
			RESULT_VARIABLE _missing OUTPUT_QUIET ERROR_QUIET)
		file(REMOVE_RECURSE "${_cgss}/${_path}")
		file(MAKE_DIRECTORY "${_cgss}/${_path}")
		if(_missing EQUAL 0)
			run(sh -c "git --git-dir='${_module_git}' archive --format=tar ${_commit} | tar -x -C '${_cgss}/${_path}'")
		else()
			execute_process(COMMAND git config -f "${OUTPUT}/litecgss.gitmodules" --get-regexp "submodule\\..*\\.path"
				OUTPUT_VARIABLE _paths OUTPUT_STRIP_TRAILING_WHITESPACE)
			string(REGEX MATCH "submodule\\.([^\n]+)\\.path ${_path}" _ "${_paths}")
			execute_process(COMMAND git config -f "${OUTPUT}/litecgss.gitmodules" --get "submodule.${CMAKE_MATCH_1}.url"
				OUTPUT_VARIABLE _url OUTPUT_STRIP_TRAILING_WHITESPACE)
			set(_fetch "${OUTPUT}/fetch/${_path}")
			file(REMOVE_RECURSE "${_fetch}")
			run(git init -q "${_fetch}")
			run(git -C "${_fetch}" fetch -q --depth 1 "${_url}" ${_commit})
			run(sh -c "git -C '${_fetch}' archive --format=tar FETCH_HEAD | tar -x -C '${_cgss}/${_path}'")
		endif()
	endforeach()
	string(SUBSTRING "${_recorded}" 0 7 _short)
	set(_cgss_note "recorded ${_short} (the checkout is at another commit)")
	message(STATUS "litergss2-reference: LiteCGSS at the recorded commit ${_short}, not the checkout's")
endif()

# 2. LiteCGSS (LiteRGSS2's), static, in-source: extconf.rb looks in external/litecgss/lib.
run(${CMAKE_COMMAND} -S "${_cgss}" -B "${_cgss}" -DBUILD_SHARED_LIBS=FALSE -DCMAKE_BUILD_TYPE=Debug
	-DLITECGSS_NO_TEST=TRUE "-DSFML_DIR=${SFML_ROOT}/lib/cmake/SFML" -DCMAKE_POSITION_INDEPENDENT_CODE=ON)
run(${CMAKE_COMMAND} --build "${_cgss}" --parallel)

# 3. The extension, as rake-compiler would: extconf.rb, then make.
set(_ext_build "${OUTPUT}/ext-build")
file(MAKE_DIRECTORY "${_ext_build}" "${OUTPUT}/lib")
execute_process(
	COMMAND ${CMAKE_COMMAND} -E env GEM_NAME=LiteRGSS EXT_NAME=LiteRGSS TRACY_ENABLED=0 "SFML_DIR=${SFML_ROOT}"
		"${RUBY}" "${_src}/ext/LiteRGSS/extconf.rb" "--with-ldflags=-Wl,-rpath,${SFML_ROOT}/lib"
	WORKING_DIRECTORY "${_ext_build}"
	RESULT_VARIABLE _status)
if(NOT _status EQUAL 0)
	message(FATAL_ERROR "litergss2-reference: extconf.rb failed (${_status}): see ${_ext_build}/mkmf.log")
endif()
run(make -C "${_ext_build}" -j8)
file(COPY_FILE "${_ext_build}/LiteRGSS.so" "${OUTPUT}/lib/LiteRGSS.so")

# 4. What was built.
execute_process(COMMAND git -C "${SOURCE}" describe --always --dirty OUTPUT_VARIABLE _rev OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git -C "${SOURCE}/external/litecgss" describe --always --dirty OUTPUT_VARIABLE _cgss_rev OUTPUT_STRIP_TRAILING_WHITESPACE)
file(WRITE "${OUTPUT}/reference.txt" "litergss2 ${_rev} (${SOURCE})\nlitecgss ${_cgss_note}; checkout ${_cgss_rev}\n")
message(STATUS "litergss2-reference: ${OUTPUT}/lib/LiteRGSS.so — litergss2 ${_rev}, litecgss ${_cgss_note}")
