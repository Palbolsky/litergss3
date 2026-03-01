# frozen_string_literal: true
require 'mkmf'

ext_name = 'LiteRGSS'

raylib_dir = File.expand_path('../../vendor/raylib', __dir__)
raylib_include = File.join(raylib_dir, 'include')
raylib_lib = File.join(raylib_dir, 'lib')

$INCFLAGS << " -I#{raylib_include}"
$LIBPATH << raylib_lib

abort "raylib.h not found" unless File.exist?(File.join(raylib_include, 'raylib.h'))
abort "libraylib.a not found" unless File.exist?(File.join(raylib_lib, 'libraylib.a'))

have_library('raylib')
$LDFLAGS << " -lraylib -lopengl32 -lgdi32 -lwinmm"

puts "C++ Compiler is #{CONFIG['CXX']}"

$CXXFLAGS << " -std=c++17 -Wall "

src_dir = __dir__

Dir.glob("#{src_dir}/**/").each do |subdir|
  next if subdir == src_dir + '/'
  rel = subdir.sub(src_dir + '/', '')
  $VPATH << "$(srcdir)/#{rel}"
  $INCFLAGS << " -I#{subdir}"
end

$srcs = Dir.glob("#{src_dir}/**/*.cpp").map { |f| f.sub(src_dir + '/', '') }.sort

create_makefile(ext_name)
