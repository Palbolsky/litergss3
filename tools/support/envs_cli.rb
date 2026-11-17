# frozen_string_literal: true

require 'optparse'

module LiteRGSSTools
  # envs — an agent's environments in this worktree: .run/<name>/.
  class EnvsCLI
    USAGE = <<~TXT
      Usage: envs <command> [options]

        up <name> [--game DIR] [--fresh] [--link a,b] [--xvfb]
                       create the env: its own copy of the game in .run/<name>/game
                       (default game: $PSDK_GAME_DIR, else ../PSDKTechnicalDemo);
                       --link: read-only folders linked, not copied (default audio,graphics);
                       --xvfb: a display of its own (Xvfb :41+) instead of hidden windows on yours
        ps             every env of this worktree, and the game running in it
        stop <name> [--timeout SEC]
                       stop its game through the quit path (SIGKILL only if that
                       fails, never SIGTERM); lists the files the run wrote
        rm <name>      delete the env and stop its Xvfb (refused while a game runs in it)

      Then: build/<preset>/bin/litergss-run --env <name>; litergss-agent --env <name> ...
    TXT

    def main(argv)
      command = argv.shift
      case command
      when 'up' then up(argv)
      when 'ps' then ps
      when 'stop' then stop(argv)
      when 'rm' then rm(argv)
      when nil, '-h', '--help' then puts(USAGE) || 0
      else raise Error, "unknown command '#{command}'\n#{USAGE}"
      end
    rescue Error, OptionParser::ParseError, SystemCallError => e
      warn "envs: #{e.message}"
      1
    end

    private

    def up(argv)
      options = { links: Env::DEFAULT_LINKS }
      OptionParser.new do |o|
        o.on('--game DIR') { |v| options[:game] = v }
        o.on('--fresh') { options[:fresh] = true }
        o.on('--link LIST', Array) { |v| options[:links] = v }
        o.on('--xvfb') { options[:xvfb] = true }
      end.parse!(argv)
      env = Env.new(argv.shift || raise(Error, 'up: env name missing'))
      game = options[:game] || ENV['PSDK_GAME_DIR'] || BUILD[:default_game]
      raise Error, "up: no game folder at #{game} (--game DIR)" unless File.directory?(game)

      started = Time.now
      outcome = env.up!(game, fresh: options[:fresh], links: options[:links])
      detail = outcome == :created ? format('copied in %.1fs, linked: %s', Time.now - started, env.links.join(',')) : 'reused (--fresh to re-copy)'
      env.start_xvfb! if options[:xvfb]
      shown = env.display ? "Xvfb :#{env.display['number']} (pid #{env.display['pid']})" : 'hidden-window ($DISPLAY)'
      puts "#{env.name}  display #{shown}  run dir #{env.dir}"
      puts "#{env.name}  game copy #{env.game_dir} ← #{env.game_source}  (#{detail})"
      puts "#{env.name}  socket #{env.socket_path}"
      0
    end

    def ps
      envs = Env.all
      puts 'no env in this worktree (envs up <name>)' if envs.empty?
      envs.each do |env|
        run = env.current
        if env.running?
          frame = begin
            Agent.request(env.socket_path, 'ping', timeout: 1)['frame']
          rescue Error
            '?'
          end
          puts "#{env.name}  game #{run['command']} (pid #{run['pid']}, frame #{frame}, #{run['preset']})  #{run['run_dir']}"
        elsif run
          puts "#{env.name}  game exited (pid #{run['pid']}) — `envs stop #{env.name}` records its files"
        else
          puts "#{env.name}  idle  game copy of #{env.game_source}#{env.display ? "  Xvfb :#{env.display['number']}" : ''}"
        end
      end
      0
    end

    def stop(argv)
      options = { timeout: 15 }
      OptionParser.new { |o| o.on('--timeout SEC', Float) { |v| options[:timeout] = v } }.parse!(argv)
      env = existing(argv.shift)
      puts env.stop!(timeout: options[:timeout])
      0
    end

    def rm(argv)
      env = existing(argv.shift)
      env.remove!
      puts "#{env.name}: removed"
      0
    end

    def existing(name)
      raise Error, 'env name missing' unless name

      env = Env.new(name)
      raise Error, "no env '#{name}' in this worktree (envs ps)" unless env.exist?

      env
    end
  end
end
