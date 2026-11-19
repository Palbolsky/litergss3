# frozen_string_literal: true

require 'open3'
require 'socket'
require 'optparse'

module LiteRGSSTools
  # litergss-run — start a game in an env, on this build, and wait until it answers.
  class RunCLI
    USAGE = <<~TXT
      Usage: litergss-run --env NAME [--entry FILE] [--no-build] [--timeout SEC] [--visible] [-- game args...]

      Starts the env's game copy on this build's LiteRGSS (#{BUILD[:preset]}, #{BUILD[:backend]}),
      hidden window, agent channel on, engine logs in the run folder; returns
      once the game answers `run state` (or fails, and says why).
        --entry FILE   script to run in the game folder (default: Game.rb)
        --no-build     skip the incremental build of the extension
        --timeout SEC  how long to wait for the game to answer (default 120)
        --visible      show the window (it is hidden by default)
        --dump-frames [N]  write the presented frames to the run's frames/ folder: one in N,
                       or (no N) each time frames/request is created
      Stop it with: envs stop NAME. Refuses a release build and the original game folder.
    TXT

    def main(argv)
      options = { entry: 'Game.rb', build: true, timeout: 120 }
      parser = OptionParser.new do |o|
        o.banner = USAGE
        o.on('--env NAME') { |v| options[:env] = v }
        o.on('--entry FILE') { |v| options[:entry] = v }
        o.on('--no-build') { options[:build] = false }
        o.on('--timeout SEC', Float) { |v| options[:timeout] = v }
        o.on('--visible') { options[:visible] = true }
        o.on('--dump-frames [N]', Integer) { |v| options[:dump_frames] = v || 0 }
      end
      parser.parse!(argv)
      if options[:env].nil?
        puts USAGE
        return 2
      end
      run(Env.new(options[:env]), options, argv)
    rescue Error, OptionParser::ParseError, SystemCallError => e
      warn "litergss-run: #{e.message}"
      1
    end

    private

    def run(env, options, game_args)
      check_build!
      raise Error, "no env '#{env.name}' (build/#{BUILD[:preset]}/bin/envs up #{env.name})" unless env.exist?
      raise Error, "#{env.name}: a game already runs (pid #{env.current['pid']}) — envs stop #{env.name}" if env.running?
      env.finish_run!(env.current) if env.current # an earlier run exited on its own
      check_game_copy!(env)
      entry = File.join(env.game_dir, options[:entry])
      raise Error, "no #{options[:entry]} in #{env.game_dir} (--entry FILE)" unless File.file?(entry)

      build! if options[:build]
      extension = Dir.glob(File.join(BUILD[:ext_dir], "#{BUILD[:ext_name]}.{so,bundle,dll}")).first
      raise Error, "no #{BUILD[:ext_name]} extension in #{BUILD[:ext_dir]} (build first)" unless extension

      run_dir = env.new_run_dir!
      File.write(File.join(run_dir, 'snapshot.json'), JSON.generate(Files.snapshot(env.game_dir, skip: env.links)))
      started_at = Time.now
      pid = spawn_game(env, run_dir, options, game_args)
      env.data['current'] = { 'pid' => pid, 'marker' => options[:entry], 'command' => "ruby #{options[:entry]}",
                              'run_dir' => run_dir, 'preset' => BUILD[:preset], 'started_at' => started_at.iso8601 }
      env.data['current']['tracy_port'] = @tracy_port if @tracy_port
      env.save!
      puts "LiteRGSS3 #{extension} (backend #{BUILD[:backend]}, preset #{BUILD[:preset]}) on #{env.game_dir}"
      wait_ready(env, pid, run_dir, started_at, options[:timeout])
    end

    def check_build!
      if BUILD[:build_type] == 'Release' || !BUILD[:agent]
        raise Error, "build '#{BUILD[:preset]}' has no agent channel (LITECGSS_AGENT OFF / release): use a debug preset's litergss-run"
      end
    end

    # Agent runs only ever run in an env's copy, never in the original game folder.
    def check_game_copy!(env)
      copy = File.realpath(env.game_dir)
      root = File.realpath(LiteRGSSTools.run_root)
      raise Error, "#{copy} is not under #{root}: refusing to run" unless copy.start_with?("#{root}/")
      raise Error, "#{env.name}: game copy is the original folder: refusing to run" if copy == File.realpath(env.game_source)
    end

    def build!
      output, status = Open3.capture2e('cmake', '--build', BUILD[:binary_dir], '--target', 'LiteRGSS', '--parallel')
      return if status.success?

      raise Error, "build failed (cmake --build #{BUILD[:binary_dir]} --target LiteRGSS):\n#{output.lines.last(20).join}"
    end

    # A free loopback port for this run's profiler (Tracy's default is 8086).
    def tracy_port
      @tracy_port ||= (18_100..18_199).find do |port|
        TCPServer.new('127.0.0.1', port).close
        true
      rescue SystemCallError
        false
      end || raise(Error, 'no free port for the profiler in 18100-18199')
    end

    def spawn_game(env, run_dir, options, game_args)
      scripts = [BUILD[:agent_scripts], ENV.fetch('LITERGSS_AGENT_SCRIPTS', nil)].compact.reject(&:empty?)
      game_env = {
        'GAMEDEPS' => BUILD[:gamedeps_dir],
        'LITERGSS_LIB' => File.join(BUILD[:ext_dir], BUILD[:ext_name]),
        'LITERGSS_TITLE_SUFFIX' => " - LiteRGSS3 #{BUILD[:backend]} (#{BUILD[:preset]}, env #{env.name})",
        # Hidden on the session's display; on the env's own Xvfb nobody sees it anyway.
        'LITERGSS_HIDDEN_WINDOW' => options[:visible] || env.display ? '0' : '1',
        'LITERGSS_AGENT' => env.socket_path,
        'LITERGSS_AGENT_SCRIPTS' => scripts.join(File::PATH_SEPARATOR),
        'LITERGSS_AGENT_SHOTS' => File.join(run_dir, 'shots'),
        'LITECGSS_LOG_DIR' => File.join(run_dir, 'logs'),
        'DISPLAY' => env.display_env
      }
      # A sanitizer build: its runtime preloaded into Ruby, its reports in stderr.log
      game_env.merge!(BUILD[:sanitizer_env])
      # An Xvfb has no vertical sync: the game's frame rate caps it instead.
      game_env['LITECGSS_NO_VSYNC'] = '1' if env.display
      if BUILD[:tracy]
        # The profiler of this Tracy build, started by the extension, on a port of this run's own.
        game_env['LITERGSS_TRACY_START'] = '1'
        game_env['TRACY_PORT'] = tracy_port.to_s
        game_env['TRACY_NO_SYS_TRACE'] = '1'
      end
      if options.key?(:dump_frames)
        @dump_frames = true
        frames = File.join(run_dir, 'frames')
        game_env['LITECGSS_DUMP_FRAMES'] = options[:dump_frames].positive? ? "#{frames}:every=#{options[:dump_frames]}" : frames
      end
      pid = Process.spawn(game_env, BUILD[:ruby], options[:entry], *game_args,
                          chdir: env.game_dir, in: File::NULL, pgroup: true,
                          out: [File.join(run_dir, 'stdout.log'), 'w'], err: [File.join(run_dir, 'stderr.log'), 'w'])
      Process.detach(pid)
      pid
    end

    def wait_ready(env, pid, run_dir, started_at, timeout)
      deadline = started_at + timeout
      loop do
        unless Processes.alive?(pid)
          warn "litergss-run: the game exited during startup — last lines of #{run_dir}/stderr.log:"
          warn File.readlines(File.join(run_dir, 'stderr.log')).last(15).join
          puts env.finish_run!(env.current)
          return 1
        end
        reply = try_state(env)
        if reply
          state = reply['value'] || {}
          puts format('ready: frame %s, scene %s (agent state answered in %.1f s)', reply['frame'], state['scene'] || '-', Time.now - started_at)
          puts "run dir #{run_dir}  (stdout.log, stderr.log, logs/, shots/#{File.directory?(File.join(run_dir, 'frames')) || @dump_frames ? ', frames/' : ''}; files-written.txt on stop)"
          puts "stop: build/#{BUILD[:preset]}/bin/envs stop #{env.name}"
          return 0
        end
        if Time.now > deadline
          warn "litergss-run: no answer within #{timeout}s — the game is not ready; stopping it"
          puts env.stop!(timeout: 5)
          return 1
        end
        sleep 0.25
      end
    end

    def try_state(env)
      return nil unless File.socket?(env.socket_path)

      reply = Agent.request(env.socket_path, 'run', 'state', timeout: 2)
      reply['ok'] ? reply : nil
    rescue Error
      nil
    end
  end
end
