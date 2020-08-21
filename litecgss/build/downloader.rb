require 'net/http'

module Cgss
    class Downloader
        def self.download_or_quit(uri, limit = 10, recurse = false)
            continue = true
            unless recurse
                puts "Downloading file #{uri}"
            end
            t = Thread.new do
                while continue
                    sleep 0.5
                    print "." if continue
                end
            end 
            
            Net::HTTP.start(uri.host, uri.port, 
                :use_ssl => uri.scheme == 'https', 
                :verify_mode => OpenSSL::SSL::VERIFY_NONE) do |http| 
                request = Net::HTTP::Get.new uri

                response = http.request request
                
                case response
                when Net::HTTPSuccess
                    continue = false
                    t.join
                    puts " Success"
                    yield(response)
                when Net::HTTPRedirection
                    continue = false
                    t.join
                    Downloader.download_or_quit(URI(response['location']), limit - 1, true) { |x| yield x }
                when Net::HTTPServerError
                    puts "#{response.message}: try again later?"
                    exit 1
                else
                    puts "Unable to download '#{uri}' : #{response.message}' with code #{response.code}"
                    exit 1
                end
            end
        end
    end
end