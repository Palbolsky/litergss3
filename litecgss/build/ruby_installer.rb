require 'fileutils'
require 'mkmf.rb'
require_relative 'system_env'

module Cgss
    class RubyInstaller
        CGSS_RUBY_VERSION_SUFFIX = "-1"

        def self.ridk_guard
            if SystemEnv::is_windows()
                ridk_script = find_executable('ridk')
                if ridk_script.nil?
                    puts("Unable to find ridk, please install ruby with devkit")
                    exit 1
                end
                return ridk_script
            end
            return nil
        end

    end
end