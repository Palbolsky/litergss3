require 'fileutils'
require 'mkmf.rb'
require_relative 'downloader'
require_relative 'system_env'

module Cgss
    class RubyInstaller
        CGSS_RUBY_VERSION_SUFFIX = "-1"

        def self.setup_path(ruby_main_version, ruby_minor_version, architecture)
            ruby_version = ruby_main_version + "." + ruby_minor_version
            ruby_installer_version = "rubyinstaller-#{ruby_version}#{CGSS_RUBY_VERSION_SUFFIX}-#{architecture}"
            root_dir = File.expand_path(File.dirname(__FILE__)) + "/.."

            SystemEnv.enqueue_path("#{root_dir}/generated/#{ruby_installer_version}/bin")
            SystemEnv.notify_all_running_windows()
        end

        def self.install_ruby_dev_kit(ruby_main_version, ruby_minor_version, architecture)
            ruby_version = ruby_main_version + "." + ruby_minor_version
            ruby_installer_version = "rubyinstaller-#{ruby_version}#{CGSS_RUBY_VERSION_SUFFIX}-#{architecture}"
            root_dir = File.expand_path(File.dirname(__FILE__)) + "/.."

            RubyInstaller::setup_path(ruby_main_version, ruby_minor_version, architecture)

            if Dir.exists?("#{root_dir}/generated/#{ruby_installer_version}/bin") and
               Dir.exists?("#{root_dir}/generated/#{ruby_installer_version}/msys32/mingw32/i686-w64-mingw32")
               return
            end

            Builder::seven_zip_guard()
            
            FileUtils.mkdir_p 'generated/tmp'

            ruby_archive_uri = URI("https://github.com/oneclick/rubyinstaller2/releases/download/" +
                "RubyInstaller-#{ruby_version}#{CGSS_RUBY_VERSION_SUFFIX}/#{ruby_installer_version}.7z")
            ruby_archive = root_dir + "/generated/tmp/ruby_dev_kit.7z"
            unless File.exists?(ruby_archive)
                Downloader.download_or_quit(ruby_archive_uri) do |response|
                    open(ruby_archive, "wb") do |file|
                        file.write(response.body)
                    end
                end
            end

            Dir.chdir(root_dir + "/generated/tmp/") {
                system("7z x -y ruby_dev_kit.7z -o\"#{root_dir}/generated\"")
            }

            msys2_archive_uri = URI("http://repo.msys2.org/distrib/msys2-i686-latest.tar.xz")
            msys2_archive = root_dir + "/generated/tmp/msys2.tar.xz"
            unless File.exists?(msys2_archive)
                Downloader.download_or_quit(msys2_archive_uri) do |response|
                    open(msys2_archive, "wb") do |file|
                        file.write(response.body)
                    end
                end
            end

            Dir.chdir(root_dir + "/generated/tmp/") {
                system("7z x -y msys2.tar.xz")
                system("7z x -y msys2.tar -o\"#{root_dir}/generated/#{ruby_installer_version}\"")

                Dir.chdir("#{root_dir}/generated/#{ruby_installer_version}/msys32/usr/bin") {
                    script = File.expand_path("#{root_dir}/build/msys32-mingw-x86-install.ps1").gsub("/", "\\")
                    system("powershell.exe -File \"#{script}\"")
                }
            }
            SystemEnv.notify_all_running_windows()
            system("gem install rake-compiler")
        end

        def self.ridk_enable(ridk_script)
            if SystemEnv::is_windows()
                print "Enabling ridk environment... "
                exit 1 if ridk_script.nil?
                ENV["RI_DEVKIT"] = File.expand_path(ridk_script + "/../../msys32/", Dir.pwd)
                ENV["MSYSTEM"] = "MINGW32"
                ENV["PKG_CONFIG_PATH"] = "/mingw32/lib/pkgconfig:/mingw32/share/pkgconfig"    
                ENV["ACLOCAL_PATH"] = "/mingw32/share/aclocal:/usr/share/aclocal"
                ENV["MANPATH"] = "/mingw32/share/man"
                ENV["MINGW_PACKAGE_PREFIX"] = "mingw-w64-i686"
                ENV["LANG"] = "fr_FR.UTF-8"
                
                ruby_bin_dir = ENV["RI_DEVKIT"] + "/../bin"
                mingw_bin_dir = ENV["RI_DEVKIT"] + "/mingw32/bin"
                msys_usr_bin_dir = ENV["RI_DEVKIT"] + "/usr/bin"
                ENV["PATH"] = ruby_bin_dir + ";" + mingw_bin_dir + ";" + msys_usr_bin_dir + ";" + ENV["PATH"]
                puts "OK"
            end
        end

        def self.ridk_guard
            if SystemEnv::is_windows()
                ridk_script = find_executable('ridk')
                if ridk_script.nil?
                    puts("Unable to find ridk, please run 'rake configure' to setup a working environment")
                    exit 1
                end
                return ridk_script
            end
            return nil
        end

        def self.detect_ridk_or_download_it(ruby_main_version, ruby_minor_version, architecture)
            if SystemEnv::is_windows()
                ridk_script = find_executable('ridk')
                if ridk_script.nil?
                    RubyInstaller.install_ruby_dev_kit(ruby_main_version, ruby_minor_version, architecture)

                    ridk_script = find_executable('ridk')
                    if ridk_script.nil?
                        puts("Still unable to find ridk, try to restart you CMD.EXE process and ensure you have it in the PATH environment variable. Aborting")
                        exit 1
                    end
                end
                return ridk_script
            end
            return nil
        end
    end
end