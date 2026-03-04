require 'mkmf'
require_relative 'ruby_installer'

module Cgss
    class Builder
        def self.cmake_guard
            !find_executable('cmake').nil? or fail "Unable to find CMake in PATH. Please, install it manually and ensure that 'cmake' is accessible in PATH before retrying. Aborting."
        end

        def self.get_cmake_generator()
            generator = ENV["LITECGSS_CMAKE_GENERATOR"] || ""
            return generator.empty? ? "" : "-G \"" + generator + "\""
        end

        def self.check_gnu_compiler()
            if SystemEnv::is_windows()
                compiler_make = find_executable("mingw32-make")
                compiler_compiler = find_executable("i686-w64-mingw32-gcc")
                compiler_compiler_pp = find_executable("i686-w64-mingw32-g++")

                if compiler_make.nil? || compiler_compiler.nil? || compiler_compiler_pp.nil?
                    ruby_executable = find_executable("ruby")
                    puts "Error : either you didn't enable the ridk with 'ridk enable', " +
                    "or you have an actual Ruby version (in #{ruby_executable}) that does not contain devkit."
                    puts "Please, manually install a compatible Ruby version with devkit provided and try again."
                    exit 0
                end
            else
                compiler_make = find_executable("make")
                compiler_compiler = find_executable("cc")
                compiler_compiler_pp = find_executable("c++")
            end
            if compiler_make.nil? || compiler_compiler.nil? || compiler_compiler_pp.nil?
                puts("Unable to find a correct compiler suite. Install one before retrying.")
                exit 1
            end
        end

    end
end
