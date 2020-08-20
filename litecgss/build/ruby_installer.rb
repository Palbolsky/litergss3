require 'fileutils'
require 'mkmf.rb'
require_relative 'downloader'
require_relative 'system_env'

module Cgss
    class RubyInstaller
        CGSS_RUBY_VERSION_SUFFIX = "-1"

        def self.install_ruby_dev_kit (ruby_main_version, ruby_minor_version, architecture)
            ruby_version = ruby_main_version + "." + ruby_minor_version
            ruby_dev_kit_uri = URI("https://github.com/oneclick/rubyinstaller2/releases/download/RubyInstaller-" + 
                + "#{ruby_version}#{CGSS_RUBY_VERSION_SUFFIX}/rubyinstaller-devkit-#{ruby_version}#{CGSS_RUBY_VERSION_SUFFIX}-#{architecture}.exe")
            
            FileUtils.mkdir_p 'generated/tmp'
            ruby_dev_kit_installer = Dir.pwd + "/generated/tmp/ruby_dev_kit.exe"
            unless File.exists?(ruby_dev_kit_installer)
                Downloader.download_or_quit(ruby_dev_kit_uri) do |response|
                    open(ruby_dev_kit_installer, "wb") do |file|
                        file.write(response.body)
                    end
                end
            end

            # This triggers Microsoft Installer. Next Next Next Install Finish. Sad.
            # TODO Windows auto install ??? How ?
            exec(ruby_dev_kit_installer)
            # Refresh path
            SystemEnv.refresh_path()
            SystemEnv.notify_all_running_windows()
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