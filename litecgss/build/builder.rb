require 'mkmf'
require_relative 'ruby_installer'

module Cgss
    class Builder
        def self.cmake_guard
            if find_executable('cmake').nil?
                puts "Unable to find CMake in PATH. Please, install it manually and ensure that 'cmake' is accessible in PATH before retrying. Aborting."
                exit 1
            end
        end

        def self.check_compiler(ruby_main_version, ruby_minor_version, architecture)
            if SystemEnv::is_windows()
                compiler_make = find_executable("mingw32-make")
                compiler_compiler = find_executable("i686-w64-mingw32-gcc")
                compiler_compiler_pp = find_executable("i686-w64-mingw32-g++")

                if compiler_make.nil? || compiler_compiler.nil? || compiler_compiler_pp.nil?
                    puts("Unable to find a correct Mingw-w64 x86 MSYS build (included in Ruby-DevKit). Try reinstalling Ruby-DevKit WITH MinGW installation checked.")
                    RubyInstaller.install_ruby_dev_kit(ruby_main_version, ruby_minor_version, architecture)
                    exit 1
                end
            else
                compiler_make = find_executable("make")
                compiler_compiler = find_executable("cc")
                compiler_compiler_pp = find_executable("c++")
                if compiler_make.nil? || compiler_compiler.nil? || compiler_compiler_pp.nil?
                    puts("Unable to find a correct compiler suite. Install one before retrying.")
                    exit 1
                end
            end
        end

    end
end
