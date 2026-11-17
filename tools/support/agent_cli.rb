# frozen_string_literal: true

require 'optparse'
require 'shellwords'

module LiteRGSSTools
  # litergss-agent — talk to a running game through its agent channel.
  class AgentCLI
    USAGE = <<~TXT
      Usage: litergss-agent [--env NAME | --socket PATH] [--timeout SEC] <request>

      Requests (answered between two frames, on the game's own thread):
        '<ruby code>'           eval in the game: {"ok":true,"value":"<inspect>","stdout":"..."}
        run <script> [args...]  a script from tools/agent/ (or one the game registered)
        scripts                 the scripts the game can run
        ping                    is the channel up? (answered by the engine)

      Prints one JSON line. Exit 0 when "ok" is true, 1 when the game answered
      an error, 2 when it could not be reached.
      --env defaults to the only env of this worktree (build/<preset>/bin/envs ps).
    TXT

    def main(argv)
      options = { timeout: 10 }
      parser = OptionParser.new do |o|
        o.banner = USAGE
        o.on('--env NAME') { |v| options[:env] = v }
        o.on('--socket PATH') { |v| options[:socket] = v }
        o.on('--timeout SEC', Float) { |v| options[:timeout] = v }
      end
      parser.order!(argv)
      if argv.empty?
        puts USAGE, '', "Scripts in #{BUILD[:agent_scripts]}:", script_list
        return 0
      end

      verb, payload = request_of(argv)
      reply = Agent.request(socket_of(options), verb, payload, timeout: options[:timeout])
      puts JSON.generate(reply)
      reply['ok'] ? 0 : 1
    rescue Error, OptionParser::ParseError => e
      warn "litergss-agent: #{e.message}"
      2
    end

    private

    def request_of(argv)
      case argv.first
      when 'run' then ['run', Shellwords.join(argv.drop(1))]
      when 'scripts', 'ping' then [argv.first, '']
      else ['eval', argv.join(' ')]
      end
    end

    def socket_of(options)
      return options[:socket] if options[:socket]
      return Env.new(options[:env]).socket_path if options[:env]

      envs = Env.all
      return envs.first.socket_path if envs.size == 1

      raise Error, (envs.empty? ? 'no env in this worktree (build/<preset>/bin/envs up <name>)' : "several envs (#{envs.map(&:name).join(', ')}): pass --env")
    end

    def script_list
      Dir.glob(File.join(BUILD[:agent_scripts], '*.rb')).sort.map do |path|
        summary = File.foreach(path).find { |line| line.start_with?('#') }.to_s.sub(/\A#\s*/, '').strip
        format('  %-8s %s', File.basename(path, '.rb'), summary)
      end
    end
  end
end
