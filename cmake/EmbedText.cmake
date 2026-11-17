# Writes a text file as a C++ string constant.
#
#   cmake -DINPUT=<file> -DOUTPUT=<header> -DNAME=<identifier> -P cmake/EmbedText.cmake
#
# The header defines `static const char* const <NAME>` holding INPUT's text
# (a raw string literal; INPUT must not contain the delimiter `)__EMBED__"`).

file(READ "${INPUT}" _text)
string(FIND "${_text}" ")__EMBED__\"" _clash)
if(NOT _clash EQUAL -1)
	message(FATAL_ERROR "${INPUT} contains the raw string delimiter )__EMBED__\"")
endif()
file(WRITE "${OUTPUT}.tmp" "// Generated from ${INPUT} by cmake/EmbedText.cmake — do not edit.\n#pragma once\nstatic const char* const ${NAME} = R\"__EMBED__(${_text})__EMBED__\";\n")
file(COPY_FILE "${OUTPUT}.tmp" "${OUTPUT}" ONLY_IF_DIFFERENT)
file(REMOVE "${OUTPUT}.tmp")
