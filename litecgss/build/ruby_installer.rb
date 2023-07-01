require 'fileutils'
require 'mkmf.rb'
require_relative 'downloader'
require_relative 'system_env'

module Cgss
    class RubyInstaller
        CGSS_RUBY_VERSION_SUFFIX = "-1"

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

        def self.check_ridk(ruby_main_version, ruby_minor_version, architecture)
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
    end
end