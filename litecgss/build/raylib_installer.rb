require_relative 'system_env'

module Cgss
  module RaylibInstaller
    module_function

    def installed?
      # raylib's `cmake --install` lays down raylib.pc; probing via
      # pkg-config avoids a real link test (which would also require
      # GL/X11 flags on linux).
      return true if system("pkg-config --exists raylib > /dev/null 2>&1")

      candidates =
        if SystemEnv::is_windows
          ['C:/raylib/lib/libraylib.a', 'C:/raylib/lib/raylib.lib',
           'C:/Program Files/raylib/lib/libraylib.a']
        else
          ['/usr/local/lib/libraylib.so', '/usr/local/lib/libraylib.a',
           '/usr/lib/libraylib.so', '/usr/lib/x86_64-linux-gnu/libraylib.so',
           '/opt/homebrew/lib/libraylib.dylib']
        end
      candidates.any? { |p| File.exist?(p) }
    end

    def check!
      return if installed?
      abort <<~MSG
        ERROR: libraylib was not found on this system.
        Install raylib system-wide before running `rake configure`:
          git clone https://github.com/raysan5/raylib
          cd raylib && mkdir build && cd build
          cmake -DBUILD_SHARED_LIBS=ON .. && sudo cmake --install .
        Or point CMake at an existing install with
        `-Draylib_DIR=/path/to/lib/cmake/raylib`.
      MSG
    end
  end
end
