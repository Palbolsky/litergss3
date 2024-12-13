require_relative 'downloader'
require_relative 'builder'
require_relative 'system_env'
require 'fileutils'

module Cgss
  module SfmlInstaller
    module_function

    def get_sfml_dir
      return ENV["SFML_DIR"] || File.join(File.dirname(__FILE__), '/../external/sfml')
    end

    def check_sfml
      sfml_dir_env = SfmlInstaller::get_sfml_dir()

      with_ldflags("-L'#{sfml_dir_env}/lib' -L'#{sfml_dir_env}/build/lib'") do
        sfml_graphics_dir = have_library('sfml-graphics') || have_library('sfml-graphics-s')
        sfml_window_dir = have_library('sfml-window') || have_library('sfml-window-s')
        sfml_system_dir = have_library('sfml-system') || have_library('sfml-system-s')

        unless sfml_graphics_dir && sfml_window_dir && sfml_system_dir
          puts "Unable to find SFML libs. Make sure you have SFML all dependencies installed if you are on Linux (you can check those on the official website). Aborting"
          exit 1
        end
      end
      return sfml_dir_env
    end

    def setup_flags
      sfml_dir_env = check_sfml
      append_ldflags("-L'#{sfml_dir_env}/lib' -L'#{sfml_dir_env}/build/lib'")
      $INCFLAGS << " -I'#{sfml_dir_env}/include'"
    end
  end
end
