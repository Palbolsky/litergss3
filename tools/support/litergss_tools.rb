# frozen_string_literal: true

# Shared code of the agent tooling (build/<preset>/bin/{litergss-agent,envs,litergss-run}).
# LiteRGSSTools::BUILD (this build's facts) is loaded first by each CLI.
#
#   Env      an agent's environment: .run/<name>/ in this worktree — its game
#            copy, its agent socket, its runs; env.json is its registry
#   Agent    the client side of LiteCGSS' agent channel (one request per connection)
#   Processes process checks; the only signal ever sent to a game is SIGKILL,
#            and only when the quit path failed (SIGTERM opens PSDK's error window)
#   Files    snapshot / diff of a folder, to list what a run wrote

require 'fileutils'
require 'json'
require 'socket'
require 'timeout'
require 'time'

module LiteRGSSTools
  class Error < StandardError; end

  def self.run_root
    File.join(BUILD[:source_dir], '.run')
  end

  # The agent channel, client side: `<verb>\n<payload>`, half-close, read the reply.
  module Agent
    module_function

    # Returns the parsed reply (a Hash). Raises Error when the game cannot be
    # reached or does not answer within `timeout` seconds.
    def request(socket_path, verb, payload = '', timeout: 10)
      raise Error, "no agent socket at #{socket_path} (game not running, or started without the agent)" unless File.socket?(socket_path)

      Timeout.timeout(timeout) do
        UNIXSocket.open(socket_path) do |socket|
          socket.write("#{verb}\n#{payload}")
          socket.close_write
          raw = socket.read
          raise Error, 'the game closed the connection without a reply' if raw.nil? || raw.empty?

          JSON.parse(raw)
        end
      end
    rescue Timeout::Error
      raise Error, "no reply within #{timeout}s (the game serves requests between frames: is it stuck, or loading?)"
    rescue Errno::ECONNREFUSED, Errno::ENOENT
      raise Error, "nothing listens on #{socket_path} (game not running)"
    end
  end

  module Processes
    module_function

    def alive?(pid, marker = nil)
      return false unless pid

      cmdline = "/proc/#{pid}/cmdline"
      if File.exist?(cmdline)
        state = File.read("/proc/#{pid}/stat").split(') ', 2).last.to_s[0]
        return false if state == 'Z'
        return marker.nil? || File.binread(cmdline).include?(marker)
      end
      Process.kill(0, pid)
      true
    rescue Errno::ESRCH, Errno::ENOENT
      false
    rescue Errno::EPERM
      true
    end

    # Waits until `pid` is gone; true if it is.
    def wait_gone(pid, seconds, marker = nil)
      deadline = Time.now + seconds
      while Time.now < deadline
        return true unless alive?(pid, marker)

        sleep 0.1
      end
      !alive?(pid, marker)
    end

    def kill9(pid)
      Process.kill('KILL', pid)
    rescue Errno::ESRCH
      nil
    end
  end

  module Files
    module_function

    # { relative path => [size, mtime_ns] } of every regular file under `root`,
    # not following symlinked folders (listed in `skip`).
    def snapshot(root, skip: [])
      result = {}
      walk(root, '', skip) do |relative, stat|
        result[relative] = [stat.size, stat.mtime.to_r.*(1_000_000_000).to_i]
      end
      result
    end

    def walk(root, prefix, skip, &block)
      Dir.each_child(File.join(root, prefix)) do |name|
        relative = prefix.empty? ? name : File.join(prefix, name)
        next if skip.include?(relative)

        path = File.join(root, relative)
        stat = File.lstat(path)
        if stat.directory?
          walk(root, relative, skip, &block)
        elsif stat.file?
          block.call(relative, stat)
        end
      end
    end

    # Lines "+ path" (created), "~ path" (modified), "- path" (removed).
    def diff(before, after)
      lines = []
      after.each do |path, info|
        if !before.key?(path) then lines << "+ #{path}"
        elsif before[path] != info then lines << "~ #{path}"
        end
      end
      before.each_key { |path| lines << "- #{path}" unless after.key?(path) }
      lines.sort_by { |line| line[2..] }
    end

    # Files under `dir` (followed through symlinks) modified after `time`.
    def modified_since(dir, time)
      return [] unless File.directory?(dir)

      Dir.glob(File.join(dir, '**', '*'), File::FNM_DOTMATCH).select do |path|
        File.file?(path) && File.mtime(path) > time
      end
    end
  end

  # An agent's environment: .run/<name>/ — env.json, game/ (the game copy),
  # agent.sock, run-NNNN/ (one per litergss-run).
  class Env
    NAME = /\A[A-Za-z0-9][A-Za-z0-9_.-]{0,31}\z/
    # Read-only assets: linked into the copy instead of copied.
    DEFAULT_LINKS = %w[audio graphics].freeze
    # Never copied: version control, logs a previous run left in the game folder.
    EXCLUDED = /\A(\.git|.*\.log)\z/

    attr_reader :name, :dir

    def self.all
      return [] unless File.directory?(LiteRGSSTools.run_root)

      Dir.children(LiteRGSSTools.run_root).sort.filter_map do |name|
        env = new(name)
        env if env.exist?
      end
    end

    def self.validate_name!(name)
      raise Error, "invalid env name '#{name}' (letters, digits, _ . -; at most 32)" unless NAME.match?(name.to_s)
    end

    def initialize(name)
      Env.validate_name!(name)
      @name = name
      @dir = File.join(LiteRGSSTools.run_root, name)
    end

    def exist?
      File.file?(registry_path)
    end

    def registry_path = File.join(dir, 'env.json')
    def socket_path = File.join(dir, 'agent.sock')
    def game_dir = File.join(dir, 'game')

    def data
      @data ||= exist? ? JSON.parse(File.read(registry_path)) : {}
    end

    def save!
      FileUtils.mkdir_p(dir)
      File.write("#{registry_path}.tmp", JSON.pretty_generate(data))
      File.rename("#{registry_path}.tmp", registry_path)
    end

    def game_source = data['game_source']
    def display = data['display'] # {"kind":"xvfb","number":41,"pid":...} or nil (hidden windows)

    # The DISPLAY the env's processes use: its own Xvfb, else the session's.
    def display_env
      display ? ":#{display['number']}" : ENV.fetch('DISPLAY', nil)
    end

    # Level 2: an Xvfb display of its own (numbers from 41 up), started now.
    def start_xvfb!
      xvfb = ENV['PATH'].split(File::PATH_SEPARATOR).map { |d| File.join(d, 'Xvfb') }.find { |f| File.executable?(f) }
      raise Error, 'Xvfb is not installed (sudo apt install xvfb), or use hidden windows (no --xvfb)' unless xvfb
      return display if display && Processes.alive?(display['pid'], 'Xvfb')

      number = (41..99).find { |n| !File.exist?("/tmp/.X#{n}-lock") && !File.exist?("/tmp/.X11-unix/X#{n}") }
      raise Error, 'no free X display between :41 and :99' unless number

      log = File.join(dir, 'xvfb.log')
      pid = Process.spawn(xvfb, ":#{number}", '-screen', '0', '1920x1080x24', '-nolisten', 'tcp',
                          in: File::NULL, out: log, err: log, pgroup: true)
      Process.detach(pid)
      deadline = Time.now + 5
      sleep 0.05 until File.exist?("/tmp/.X11-unix/X#{number}") || Time.now > deadline || !Processes.alive?(pid)
      raise Error, "Xvfb :#{number} did not start (#{log})" unless Processes.alive?(pid) && File.exist?("/tmp/.X11-unix/X#{number}")

      data['display'] = { 'kind' => 'xvfb', 'number' => number, 'pid' => pid }
      save!
      display
    end

    # Xvfb is not a game: SIGTERM is its normal stop.
    def stop_xvfb!
      return unless display

      Process.kill('TERM', display['pid']) if Processes.alive?(display['pid'], 'Xvfb')
      data.delete('display')
      save!
    rescue Errno::ESRCH
      data.delete('display')
      save!
    end
    def links = data['links'] || []
    def current = data['current']

    def running?
      current && Processes.alive?(current['pid'], current['marker'])
    end

    # Creates (or with fresh: re-creates) the game copy.
    def up!(game_source, fresh: false, links: DEFAULT_LINKS)
      game_source = File.realpath(game_source)
      raise Error, "#{name}: a game is running in this env — `envs stop #{name}` first" if running?
      @data = nil unless exist?
      if exist? && File.directory?(game_dir) && !fresh
        raise Error, "#{name} copies #{self.game_source}, not #{game_source} — use --fresh to replace it" if self.game_source != game_source

        return :reused
      end

      FileUtils.mkdir_p(dir, mode: 0o700)
      File.chmod(0o700, dir)
      FileUtils.rm_rf(game_dir)
      copy_game(game_source, game_dir, links)
      previous_display = exist? ? data['display'] : nil
      @data = { 'name' => name, 'game_source' => game_source, 'links' => links.select { |l| File.directory?(File.join(game_source, l)) },
                'created_at' => Time.now.iso8601, 'runs' => 0 }
      @data['display'] = previous_display if previous_display
      save!
      :created
    end

    # The next run folder (run-0001, ...).
    def new_run_dir!
      data['runs'] = data.fetch('runs', 0) + 1
      save!
      path = File.join(dir, format('run-%04d', data['runs']))
      FileUtils.mkdir_p(File.join(path, 'logs'))
      FileUtils.mkdir_p(File.join(path, 'shots'))
      path
    end

    def remove!
      raise Error, "#{name}: a game is running in this env — `envs stop #{name}` first" if running?

      stop_xvfb!
      root = File.realpath(LiteRGSSTools.run_root)
      target = File.realpath(dir)
      raise Error, "refusing to remove #{target}: not under #{root}" unless target.start_with?("#{root}/")

      FileUtils.rm_rf(target)
    end

    # Stops the running game through the quit path; SIGKILL only if it does
    # not exit (and says so). Then writes the run's files-written.txt.
    # Returns the lines to print.
    def stop!(timeout: 15)
      run = current
      return ["#{name}: no game running"] unless run

      lines = []
      if Processes.alive?(run['pid'], run['marker'])
        begin
          Agent.request(socket_path, 'run', 'quit', timeout: [timeout, 5].min)
          lines << "#{name}: quit sent"
        rescue Error => e
          lines << "#{name}: quit not delivered (#{e.message})"
        end
        if Processes.wait_gone(run['pid'], timeout, run['marker'])
          lines << "#{name}: game exited"
        else
          Processes.kill9(run['pid'])
          Processes.wait_gone(run['pid'], 5, run['marker'])
          lines << "#{name}: game did not exit within #{timeout}s — SIGKILLed (pid #{run['pid']})"
        end
      else
        lines << "#{name}: game already exited"
      end
      lines.concat(finish_run!(run))
      lines
    end

    # Records what the run wrote; clears `current`.
    def finish_run!(run)
      lines = []
      snapshot_path = File.join(run['run_dir'], 'snapshot.json')
      if File.file?(snapshot_path)
        before = JSON.parse(File.read(snapshot_path))
        changes = Files.diff(before, Files.snapshot(game_dir, skip: links))
        File.write(File.join(run['run_dir'], 'files-written.txt'), changes.map { |l| "#{l}\n" }.join)
        started = Time.iso8601(run['started_at'])
        through_links = links.flat_map { |l| Files.modified_since(File.join(game_dir, l), started) }
        lines << "#{name}: #{changes.size} file(s) written in the game copy → #{File.join(run['run_dir'], 'files-written.txt')}"
        changes.first(10).each { |l| lines << "  #{l}" }
        unless through_links.empty?
          lines << "#{name}: WARNING — #{through_links.size} file(s) written through a linked folder, i.e. in #{game_source}:"
          through_links.first(10).each { |l| lines << "  #{l}" }
        end
      end
      data['last'] = run.merge('stopped_at' => Time.now.iso8601)
      data.delete('current')
      save!
      FileUtils.rm_f(socket_path)
      lines
    end

    private

    def copy_game(source, target, links)
      FileUtils.mkdir_p(target)
      Dir.each_child(source) do |entry|
        next if EXCLUDED.match?(entry)

        from = File.join(source, entry)
        to = File.join(target, entry)
        if links.include?(entry) && File.directory?(from)
          File.symlink(from, to)
        else
          FileUtils.cp_r(from, to, preserve: true)
        end
      end
    end
  end
end

require_relative 'agent_cli'
require_relative 'envs_cli'
require_relative 'run_cli'
require_relative 'scene_cli'
require_relative 'profile_cli'
