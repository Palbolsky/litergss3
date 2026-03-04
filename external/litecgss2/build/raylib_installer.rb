require_relative 'builder'
require_relative 'system_env'
require 'fileutils'

module Cgss
  module RaylibInstaller
    module_function

    def get_raylib_dir
      return ENV["RAYLIB_DIR"] || File.join(File.dirname(__FILE__), '../../../vendor/raylib')
    end

    def check_raylib
      raylib_dir_env = RaylibInstaller::get_raylib_dir()

      find_library('raylib', nil, raylib_dir_env + "/include", raylib_dir_env + "/lib")

      with_ldflags("-L'#{raylib_dir_env}/lib'") do
        unless have_library('raylib')
          puts "WARNING: Unable to find raylib lib. Make sure you have raylib all dependencies installed."
        end
      end
      return raylib_dir_env
    end

    def setup_flags
      raylib_dir_env = check_raylib
      append_ldflags("-L'#{raylib_dir_env}/lib'")
      $INCFLAGS << " -I'#{raylib_dir_env}/include'"
    end
  end
end
