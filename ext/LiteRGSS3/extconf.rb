# frozen_string_literal: true
require 'mkmf'

ext_name = 'LiteRGSS3'

litecgss_root_dir = File.expand_path("../../external/litecgss2", __dir__)
raylib_dir = File.expand_path('../../vendor/raylib', __dir__)
raylib_include = File.join(raylib_dir, 'include')
raylib_lib = File.join(raylib_dir, 'lib')

# LiteCGSS backend selection. Matches CGSS_BACKEND in the LiteCGSS2 CMake
# build. Defaults to raylib for back-compat (the ext/graphics and ext/fonts
# TUs still reference raylib:: directly — see step B in the upstream plan).
# Override via CGSS_BACKEND env var: `CGSS_BACKEND=sfml rake compile`.
cgss_backend = ENV.fetch('CGSS_BACKEND', 'raylib').downcase
unless %w[sfml raylib].include?(cgss_backend)
  abort "Unknown CGSS_BACKEND '#{cgss_backend}'. Must be one of: sfml, raylib."
end
puts "[LiteRGSS3] Building with CGSS_BACKEND=#{cgss_backend}"

# Include paths:
#  -I<litecgss_root>/include : public cgss headers (Events/KeyCode.h, etc.)
#  -I<litecgss_root>/src     : backend headers (Backend/ActiveBackend.h)
#  -I<raylib_include>        : raylib.h / rlgl.h (ext/{graphics,fonts} are
#                              raylib-coupled — see the matching header
#                              comments in those files)
# The legacy /usr/include/LiteCGSS2/ path was retired in the phase-1
# cleanup: every ext/ TU now includes <raylib.h> directly (no more
# namespaced wrapper) and the submodule's /LiteCGSS/ headers (no "2").
$INCFLAGS << " -I'$(srcdir)/../../'"
$INCFLAGS << " -I'" + litecgss_root_dir + "/include'"
$INCFLAGS << " -I'" + litecgss_root_dir + "/src'"
$INCFLAGS << " -I'#{raylib_include}'"
$LDFLAGS << " -L'" + litecgss_root_dir + "/bin' -L'" + litecgss_root_dir + "/lib' " + "-L/usr/i686-w64-mingw32/lib/ "
$LIBPATH << raylib_lib

# Select backend at compile time — mirrors LiteCGSS2's CMake flag.
$CXXFLAGS << " -DCGSS_BACKEND_#{cgss_backend.upcase}"

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
