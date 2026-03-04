require 'rbconfig'

module Cgss
    class SystemEnv
        def self.is_windows
            return RbConfig::CONFIG['host_os'] =~ /mswin|mingw/
        end
    end
end
