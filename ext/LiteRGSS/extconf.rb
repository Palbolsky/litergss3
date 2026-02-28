# frozen_string_literal: true
require 'mkmf'

ext_name = 'LiteRGSS'

# Raylib version to download: raylib-5.5_win32_mingw-w64

# Relative paths to raylib
raylib_dir = File.expand_path('../../vendor/raylib', __dir__)
raylib_include = File.join(raylib_dir, 'include')
raylib_lib = File.join(raylib_dir, 'lib')

# Add the paths
$INCFLAGS << " -I#{raylib_include}"
$LIBPATH << raylib_lib

# Check if raylib exists
abort "raylib.h not found" unless File.exist?(File.join(raylib_include, 'raylib.h'))
abort "libraylib.a not found" unless File.exist?(File.join(raylib_lib, 'libraylib.a'))

have_library('raylib')

# Required Windows system libraries
$LDFLAGS << ' -lraylib -lopengl32 -lgdi32 -lwinmm'

$CXXFLAGS << ' -std=c++17'

create_makefile(ext_name)