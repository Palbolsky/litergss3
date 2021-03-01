require 'mkmf'
require_relative 'ruby_installer'

module Cgss
    class Builder
        def self.cmake_guard
            !find_executable('cmake').nil? or fail "Unable to find CMake in PATH. Please, install it manually and ensure that 'cmake' is accessible in PATH before retrying. Aborting."
        end

        def self.seven_zip_guard
            !find_executable('7z').nil? or fail "You need to have 7zip installed and accessible in PATH. Aborting."
        end

        def self.get_cmake_generator()
            root_dir = File.expand_path(File.dirname(__FILE__))
            buildtype = File.read(root_dir + "/../generated/build.cache")

            cmake_generator = ""
            if !buildtype.eql? "msvc"
                cmake_generator = Cgss::SystemEnv::is_windows() ? "-G \"MinGW Makefiles\"" : ""
            end
            return cmake_generator
        end

        def self.check_gnu_compiler(ruby_main_version, ruby_minor_version, architecture)
            if SystemEnv::is_windows()
                compiler_make = find_executable("mingw32-make")
                compiler_compiler = find_executable("i686-w64-mingw32-gcc")
                compiler_compiler_pp = find_executable("i686-w64-mingw32-g++")

                if compiler_make.nil? || compiler_compiler.nil? || compiler_compiler_pp.nil?
                    ruby_executable = find_executable("ruby")
                    puts "Error : you have an actual Ruby version (in #{ruby_executable}) that does not contain devkit."
                    puts "Please, manually install Ruby #{ruby_main_version}.#{ruby_minor_version} on architecture #{architecture} with devkit provided and try again."
                    #msys32_install_path = File.expand_path(ruby_executable + "/../..")
                    #RubyInstaller::install_msys32(msys32_install_path)
                    #system("rake configure")
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

        def self.check_msvc_compiler()
            SystemEnv::is_windows() or fail "Unsupported MSVC build for the current OS"
            vswhere_dir = ENV["ProgramFiles(x86)"] + "\\Microsoft Visual Studio\\Installer\\"
            vswhere = find_executable("vswhere.exe", vswhere_dir)
            !vswhere.nil? or fail "Unable to find any Visual Studio 2017 (or higher) installation on this OS. Aborting."
            vswhere_cmd = "cd /D \"#{vswhere_dir}\" && vswhere.exe -latest -requires Microsoft.Component.MSBuild -property installationVersion"
            vs_full_version = %x[ #{vswhere_cmd} ]
            return vs_full_version.split(".")[0]
        end
    end
end
