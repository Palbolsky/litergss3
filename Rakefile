# Thin wrapper over the CMake presets (CMakePresets.json). CMake is the
# build system; these tasks only keep the historical entry points.
#
#   rake compile                 # configure + build the `debug` preset
#   PRESET=release rake compile  # any preset from CMakePresets.json
#   rake test                    # C++ + Ruby test suites (CTest)
#   rake lint                    # module boundary check
#   rake psdk                    # play ../PSDKTechnicalDemo on this build (tools/psdk-demo.sh)
#
# Generator: CMake honours the CMAKE_GENERATOR environment variable
# (e.g. CMAKE_GENERATOR="MinGW Makefiles" on Windows).

require 'fileutils'

ROOT = __dir__

def preset
  ENV.fetch('PRESET', 'debug')
end

def binary_dir
  File.join(ROOT, 'build', preset)
end

def cmake(*args)
  sh('cmake', *args)
end

desc 'Configure the selected CMake preset (PRESET, default: debug)'
task :configure do
  Dir.chdir(ROOT) { cmake('--preset', preset) }
end

desc 'Build the selected CMake preset (PRESET, default: debug)'
task :compile do
  Rake::Task[:configure].invoke unless File.exist?(File.join(binary_dir, 'CMakeCache.txt'))
  Dir.chdir(ROOT) { cmake('--build', '--preset', preset, '--parallel') }
end

desc 'Build the release preset'
task :release do
  ENV['PRESET'] = 'release'
  Rake::Task[:compile].invoke
end

desc 'Run the C++ and Ruby test suites through CTest'
task test: :compile do
  Dir.chdir(ROOT) { sh('ctest', '--preset', preset) }
end

desc 'Check the LiteCGSS/LiteRGSS module boundaries (also run by every build)'
task :lint do
  Dir.chdir(ROOT) { cmake('-P', 'cmake/CheckBoundaries.cmake') }
end

desc 'Build, then start a PSDK game on this LiteRGSS3 (tools/psdk-demo.sh; PSDK_GAME_DIR, PRESET)'
task :psdk do
  sh(File.join(ROOT, 'tools', 'psdk-demo.sh'), '--preset', preset)
end

desc 'Remove every build tree'
task :clean do
  FileUtils.rm_rf(File.join(ROOT, 'build'))
end

task default: :compile
