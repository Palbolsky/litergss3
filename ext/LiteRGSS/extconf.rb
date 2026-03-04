# frozen_string_literal: true
require 'mkmf'

ext_name = 'LiteRGSS'

litecgss_root_dir = File.expand_path("../../external/litecgss2", __dir__)
raylib_dir = File.expand_path('../../vendor/raylib', __dir__)
raylib_include = File.join(raylib_dir, 'include')
raylib_lib = File.join(raylib_dir, 'lib')

$INCFLAGS << " -I/usr/include/LiteCGSS2/ -I'$(srcdir)/../../' -I'" + litecgss_root_dir + "/src/src' -I'#{raylib_include}'"
$LDFLAGS << " -L'" + litecgss_root_dir + "/bin' -L'" + litecgss_root_dir + "/lib' " + "-L/usr/i686-w64-mingw32/lib/ "
$LIBPATH << raylib_lib

abort "raylib.h not found" unless File.exist?(File.join(raylib_include, 'raylib.h'))
abort "libraylib.a not found" unless File.exist?(File.join(raylib_lib, 'libraylib.a'))

have_library('raylib')

dir_config('skalog', litecgss_root_dir + '/external/skalog/src/src', litecgss_root_dir + '/lib')
have_library('skalog') or fail "Unable to find skalog library. Build the LiteCGSS2 to build it."

dir_config('LiteCGSS2_engine', litecgss_root_dir + '/src/src', litecgss_root_dir + '/lib')
have_library('LiteCGSS2_engine') or fail "Unable to find LiteCGSS2 library. Build it inside 'external/litecgss2' first."

$LDFLAGS << ' -lraylib -lopengl32 -lgdi32 -lwinmm'
$CXXFLAGS << ' -std=c++17'

src_dir = __dir__

Dir.glob("#{src_dir}/**/").each do |subdir|
next if subdir == src_dir + '/'
rel = subdir.sub(src_dir + '/', '')
$VPATH << "$(srcdir)/#{rel}"
$INCFLAGS << " -I#{subdir}"
end

$srcs = Dir.glob("#{src_dir}/**/*.cpp").map { |f| f.sub(src_dir + '/', '') }.sort

create_makefile(ext_name)
