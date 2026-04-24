# frozen_string_literal: true
require 'mkmf'
require 'rbconfig'

ext_name = 'LiteRGSS'

litecgss_root_dir = File.expand_path("../../external/litecgss2", __dir__)
is_windows = !!(RbConfig::CONFIG['host_os'] =~ /mswin|mingw/)

# LiteCGSS backend selection. Matches CGSS_BACKEND in the LiteCGSS2 CMake
# build. Defaults to raylib for back-compat (the ext/graphics and ext/fonts
# TUs still reference raylib:: directly — see step B in the upstream plan).
# Override via CGSS_BACKEND env var: `CGSS_BACKEND=sfml rake compile`.
cgss_backend = ENV.fetch('CGSS_BACKEND', 'raylib').downcase
unless %w[sfml raylib].include?(cgss_backend)
  abort "Unknown CGSS_BACKEND '#{cgss_backend}'. Must be one of: sfml, raylib."
end
puts "[LiteRGSS3] Building with CGSS_BACKEND=#{cgss_backend}"

# raylib include/link flags: prefer pkg-config (system install — what
# LiteCGSS2's find_package(raylib) also picks up), fall back to a vendored
# tree at vendor/raylib/{include,lib} for CI / packaged builds.
raylib_cflags = nil
raylib_libs   = nil
if cgss_backend == 'raylib'
  if system('pkg-config --exists raylib > /dev/null 2>&1')
    raylib_cflags = `pkg-config --cflags raylib`.chomp
    raylib_libs   = `pkg-config --libs raylib`.chomp
  else
    vendored_dir = File.expand_path('../../vendor/raylib', __dir__)
    vendored_include = File.join(vendored_dir, 'include')
    vendored_lib     = File.join(vendored_dir, 'lib')
    abort "raylib.h not found: pkg-config reports no raylib.pc and no vendored tree at #{vendored_include}" \
      unless File.exist?(File.join(vendored_include, 'raylib.h'))
    abort "libraylib not found under #{vendored_lib}" \
      if Dir.glob(File.join(vendored_lib, 'libraylib.*')).empty?
    raylib_cflags = "-I'#{vendored_include}'"
    raylib_libs   = "-L'#{vendored_lib}' -lraylib"
  end
end

# Include paths:
#  -I<litecgss_root>/include : public cgss headers (Events/KeyCode.h, etc.)
#  -I<litecgss_root>/src     : backend headers (Backend/ActiveBackend.h)
#  raylib_cflags             : raylib.h / rlgl.h (ext/{graphics,fonts} are
#                              raylib-coupled — see the matching header
#                              comments in those files)
# The legacy /usr/include/LiteCGSS2/ path was retired in the phase-1
# cleanup: every ext/ TU now includes <raylib.h> directly (no more
# namespaced wrapper) and the submodule's /LiteCGSS/ headers (no "2").
$INCFLAGS << " -I'$(srcdir)/../../'"
$INCFLAGS << " -I'" + litecgss_root_dir + "/include'"
$INCFLAGS << " -I'" + litecgss_root_dir + "/src'"
$INCFLAGS << " #{raylib_cflags}" if raylib_cflags
$LDFLAGS  << " -L'" + litecgss_root_dir + "/bin' -L'" + litecgss_root_dir + "/lib'"
$LDFLAGS  << " -L/usr/i686-w64-mingw32/lib/" if is_windows

# Select backend at compile time — mirrors LiteCGSS2's CMake flag.
$CXXFLAGS << " -DCGSS_BACKEND_#{cgss_backend.upcase}"

dir_config('skalog', litecgss_root_dir + '/external/skalog/src/src', litecgss_root_dir + '/lib')
have_library('skalog') or fail "Unable to find skalog library. Build the LiteCGSS2 to build it."

dir_config('LiteCGSS_engine', litecgss_root_dir + '/src/src', litecgss_root_dir + '/lib')
have_library('LiteCGSS_engine') or fail "Unable to find LiteCGSS2 library. Build it inside 'external/litecgss2' first."

$LDFLAGS  << " #{raylib_libs}" if raylib_libs
# Windows-only system libs needed for raylib's MinGW static build.
$LDFLAGS  << ' -lopengl32 -lgdi32 -lwinmm' if is_windows && cgss_backend == 'raylib'
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
