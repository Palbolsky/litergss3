require_relative 'downloader'
require_relative 'builder'
require_relative 'system_env'
require 'fileutils'

module Cgss
    class SfmlInstaller

        def self.get_sfml_dir()
            sfml_dir_env = ENV["SFML_DIR"]
            if sfml_dir_env.nil?
                sfml_dir_env = File.dirname(__FILE__) + "/../external/sfml"
            end
            return sfml_dir_env
        end

        def self.check_sfml()
            sfml_dir_env = SfmlInstaller::get_sfml_dir()

            sfml_graphics_dir = have_library('sfml-graphics')
            sfml_window_dir = have_library('sfml-window')
            sfml_system_dir = have_library('sfml-system')

            if !sfml_graphics_dir || !sfml_window_dir || !sfml_system_dir
                puts "Unable to find SFML libs. Make sure you have SFML all dependencies installed if you are on Linux (you can check those on the official website). Aborting"
                exit 1
            end
            return sfml_dir_env
        end

    end
end