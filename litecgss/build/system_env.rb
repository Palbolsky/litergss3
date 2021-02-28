require 'rbconfig'

module Cgss
    class SystemEnv
        def self.is_windows
            return RbConfig::CONFIG['host_os'] =~ /mswin|mingw/
        end

        def self.export_env(variable, value)
            if SystemEnv::is_windows()
                require 'win32/registry.rb'
                Win32::Registry::HKEY_CURRENT_USER.open('Environment', Win32::Registry::KEY_WRITE) do |reg|
                    reg[variable] = value
                end
            else
                puts "Unsupported export env on platform '" + RbConfig::CONFIG['host_os'] + "'"
            end
        end

        def self.notify_all_running_windows
            if SystemEnv::is_windows()
                # Win32API has been removed since Ruby 3.0.0
                #require 'Win32API'
                #sendMessageTimeout = Win32API.new('user32', 'SendMessageTimeout', 'LLLPLLP', 'L') 
                #hwnd_broadcast = 0xffff
                #wm_settingchange = 0x001A
                #smto_abortifhung = 2
                #result = 0
                #sendMessageTimeout.call(hwnd_broadcast, wm_settingchange, 0, 'Environment', smto_abortifhung, 5000, result) 
                # In case of PowerShell + Chocolatey
                system("refreshenv")
            else
                puts "Unsupported env notify all windows on platform '" + RbConfig::CONFIG['host_os'] + "'"
            end
        end

        def self.enqueue_path(value)
            if SystemEnv::is_windows()
                value = value.gsub!("/", "\\")
                export_env("PATH", "#{value};" + ENV["PATH"])
                ENV["PATH"] = "#{value};" + ENV["PATH"]
            else
                system("export 'PATH=#{value}:$PATH'")
            end
        end
    end
end
