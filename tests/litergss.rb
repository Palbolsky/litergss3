# Loads the LiteRGSS extension under test: the one CTest points at through
# LITERGSS_LIB (each build directory's own output), else build/lib's, where
# `rake compile` / the debug preset put it.
require File.expand_path(ENV['LITERGSS_LIB'] || File.join(__dir__, '..', 'build', 'lib', 'LiteRGSS'))
