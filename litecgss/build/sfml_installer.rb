require_relative 'downloader'
require_relative 'builder'
require_relative 'system_env'
require 'fileutils'

module Cgss
    class SfmlInstaller
        
        def self.clean_make_sfml(sfml_src)
            Builder.cmake_guard()
            puts "Building SFML in #{sfml_src}... "
            Dir.chdir(sfml_src) {
                File.delete("CMakeCache.txt") if File.exists? 'CMakeCache.txt'
                if SystemEnv::is_windows()
                    system("cmake -G \"MinGW Makefiles\" -DSFML_BUILD_EXAMPLES=False .")
                    system("cmake --build .")
                else
                    system("cmake -DSFML_BUILD_EXAMPLES=False .")
                    system("cmake --build .")
                    puts "Installing newly compiled SFML"
                    system("ln -sf #{sfml_src}/lib/*.so* /usr/lib/")
                    system("ln -sf #{sfml_src}/include/SFML /usr/include/")
                end
            }
        end

        def self.install_sfml(sfml_dir, sfml_download_uri)
            sfml_src_zip = File.expand_path(sfml_dir + "/../sfml_src.zip")
            sfml_dir = File.expand_path(sfml_dir)
            # Zip querying
            unless File.exists?(sfml_src_zip) && Dir.exist?(sfml_dir + "/src")
                FileUtils.rm_rf sfml_dir
                FileUtils.mkdir_p sfml_dir
                unless File.exists?(sfml_src_zip) 
                    Downloader.download_or_quit(sfml_download_uri) do |response|
                        open(sfml_src_zip, "wb") do |file|
                            file.write(response.body)
                            SystemEnv.export_env('SFML_DIR', sfml_dir)
                            SystemEnv.notify_all_running_windows()
                        end
                    end
                end
            end

            # Sources extraction
            unless Dir.exist?(sfml_dir + "/src")
                Dir.chdir(File. expand_path(File. dirname(__FILE__)) + "/") {
                    if SystemEnv::is_windows()
                        win_sfml_src_zip = sfml_src_zip.gsub("/", "\\")
                        win_sfml_dir = sfml_dir.gsub("/", "\\")
                        win_tmp_sfml_dir = File.expand_path(win_sfml_dir + "\\..\\tmp")
                        win_tmp_sfml_dir.gsub!("/", "\\")
                        system("cscript win_unzip.vbs \"#{win_sfml_src_zip}\" \"#{win_tmp_sfml_dir}\"")
                        zip_root = File.expand_path(win_tmp_sfml_dir + "\\" + Dir.children(win_tmp_sfml_dir)[0])
                        FileUtils.mv Dir.glob(zip_root + "/*"), win_sfml_dir + "/", :force => true
                        FileUtils.rm_rf win_tmp_sfml_dir
                    else
                        tmp_sfml_dir = File.expand_path(sfml_dir + "/../tmp")
                        FileUtils.mkdir_p tmp_sfml_dir
                        system("unzip -o \"#{sfml_src_zip}\" -d \"#{tmp_sfml_dir}\"")
                        zip_root = File.expand_path(tmp_sfml_dir + "/" + Dir.children(tmp_sfml_dir)[0])
                        FileUtils.mv Dir.glob(zip_root + "/*"), sfml_dir + "/", :force => true
                        FileUtils.rm_rf tmp_sfml_dir
                    end
                }
            end
        end

        def self.check_sfml_or_download_and_make_it(sfml_version)
            sfml_dir_env = ENV["SFML_DIR"]
            if sfml_dir_env.nil?
                sfml_dir_env = File.dirname(__FILE__) + "/../external/sfml"
            else
                puts "SFML found from SFML_DIR (#{sfml_dir_env})"    
            end

            $INCFLAGS << " -I" + (sfml_dir_env + "/include")
            $LDFLAGS << " -L" + (sfml_dir_env + "/lib")

            sfml_graphics_dir = have_library('sfml-graphics')
            sfml_window_dir = have_library('sfml-window')
            sfml_system_dir = have_library('sfml-system')

            if !sfml_graphics_dir || !sfml_window_dir || !sfml_system_dir
                install_sfml(sfml_dir_env, URI("https://www.sfml-dev.org/files/SFML-#{sfml_version}-sources.zip"))
                clean_make_sfml(sfml_dir_env)
                sfml_graphics_dir = have_library('sfml-graphics')
                sfml_window_dir = have_library('sfml-window')
                sfml_system_dir = have_library('sfml-system')
                if !sfml_graphics_dir || !sfml_window_dir || !sfml_system_dir
                    puts "Unable to find SFML libs. Make sure you have SFML all dependencies installed if you are on Linux (you can check those on the official website). Aborting"
                    exit 1
                end
            end
        end
    end
end