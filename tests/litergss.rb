# Loads the LiteRGSS extension under test: the one CTest points at through
# LITERGSS_LIB (each build directory's own output), else the debug preset's
# (what `rake compile` builds), else build/lib (a build without preset).
candidates = [
  File.join(__dir__, '..', 'build', 'debug', 'lib', 'LiteRGSS'),
  File.join(__dir__, '..', 'build', 'lib', 'LiteRGSS')
]
default = candidates.find { |path| !Dir.glob("#{path}.{so,bundle,dll}").empty? } || candidates.first
require File.expand_path(ENV['LITERGSS_LIB'] || default)
