require 'rbconfig'

module Cgss
    class SystemEnv
        IS_WINDOWS = (RbConfig::CONFIG['host_os'] =~ /mswin|mingw/)

        def self.export_env(variable, value)
            if IS_WINDOWS
                require 'win32/registry.rb'
                Win32::Registry::HKEY_CURRENT_USER.open('Environment', Win32::Registry::KEY_WRITE) do |reg|
                    reg[variable] = value
                end
            else
                puts "Unsupported export env on platform '" + RbConfig::CONFIG['host_os'] + "'"
            end
        end

        def self.notify_all_running_windows
            if IS_WINDOWS
                require 'Win32API'
                sendMessageTimeout = Win32API.new('user32', 'SendMessageTimeout', 'LLLPLLP', 'L') 
                hwnd_broadcast = 0xffff
                wm_settingchange = 0x001A
                smto_abortifhung = 2
                result = 0
                sendMessageTimeout.call(hwnd_broadcast, wm_settingchange, 0, 'Environment', smto_abortifhung, 5000, result) 
            else
                puts "Unsupported env notify all windows on platform '" + RbConfig::CONFIG['host_os'] + "'"
            end
        end

        def self.refresh_path
            if IS_WINDOWS
                system(File.dirname(__FILE__) + "/resetvars.bat")
            else
                puts "Unsupported refresh PATH variable on platform '" + RbConfig::CONFIG['host_os'] + "'"
            end
        end
    end
end
