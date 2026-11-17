# LiteRGSS::Agent runtime — embedded in the extension (AgentRuntime.h) and
# evaluated at load time when LITERGSS_AGENT names a socket. Serves the
# agent channel's verbs on the game's own thread, between two frames:
#
#   eval <ruby code>         the code's value (#inspect), and what it printed
#   run <script> [args...]   a script from the script paths; its value as JSON
#   scripts                  the scripts found, with their one-line summary
#
# Every reply: {"ok":..., "value":..., "stdout":"...", "ms":..., "frame":...}
# or {"ok":false, "error":"Class: message", "backtrace":[...]}. Nothing a
# request raises (not even `exit`) reaches the game loop.
#
# Script paths: LITERGSS_AGENT_SCRIPTS (path-separated), then any folder a
# host adds to LiteRGSS::Agent.script_paths. A script is a Ruby file whose
# last value is the result; `args` holds its arguments (strings).
module LiteRGSS
  module Agent
    @script_paths = (ENV['LITERGSS_AGENT_SCRIPTS'] || '').split(File::PATH_SEPARATOR).reject(&:empty?)
    @quit_requested = false
    @frame_times = []
    @frame = 0
    @paused = false
    @steps = 0
    @scheduled = [] # [[frame, proc]]

    class << self
      # Folders searched by `run`, in order. A host adds its own.
      attr_reader :script_paths

      # Asks the game to close at the next DisplayWindow#update, through the
      # same path as the player closing the window (on_closed, ClosedWindowError).
      def request_quit!
        @quit_requested = true
      end

      # Called by the extension; true once per quit request.
      def take_quit_request
        requested = @quit_requested
        @quit_requested = false
        requested
      end

      # Game updates (DisplayWindow#update) since the channel opened. A held
      # game does not advance it.
      attr_reader :frame

      # Called by the extension at the start of each update.
      def tick
        @frame += 1
        @frame_times << Process.clock_gettime(Process::CLOCK_MONOTONIC)
        @frame_times.shift if @frame_times.size > 240
        @steps -= 1 if @steps.positive?
        due, @scheduled = @scheduled.partition { |at, _| at <= @frame }
        due.each { |_, action| action.call }
      end

      # Runs `action` at the start of the update `frames` from now.
      def after_frames(frames, &action)
        @scheduled << [@frame + [frames, 1].max, action]
      end

      # Holds the game at its next update (no game code runs; requests are served).
      def pause!
        @paused = true
        @steps = 0
      end

      def resume!
        @paused = false
        @steps = 0
      end

      # From a held game: runs `count` updates, then holds again. Requests
      # sent meanwhile are served after the last one.
      def step!(count)
        raise ArgumentError, 'step: count must be >= 1' if count < 1

        pause!
        @steps = count
      end

      def paused? = @paused

      # Extension: hold the game in this update?
      def hold? = @paused && @steps.zero? && !@quit_requested

      # Extension: a `step` is running (do not serve requests).
      def stepping? = @paused && @steps.positive? && !@quit_requested

      # Frame durations (ms) over the last `count` frames.
      def frame_durations(count = 120)
        times = @frame_times.last(count + 1)
        times.each_cons(2).map { |a, b| (b - a) * 1000.0 }
      end

      # The open DisplayWindows, oldest first.
      def windows
        ObjectSpace.each_object(LiteRGSS::DisplayWindow).reject(&:disposed?).sort_by(&:object_id)
      end

      def scripts
        script_paths.flat_map { |dir| Dir.glob(File.join(dir, '*.rb')) }.map do |path|
          summary = File.foreach(path).find { |line| line.start_with?('#') }
          { name: File.basename(path, '.rb'), path: path, summary: summary&.sub(/\A#\s*/, '')&.strip }
        end.uniq { |script| script[:name] }
      end

      def handle(verb, payload)
        require 'json'
        require 'stringio'
        require 'shellwords'
        started = Process.clock_gettime(Process::CLOCK_MONOTONIC)
        captured = StringIO.new
        reply =
          begin
            previous_stdout = $stdout
            $stdout = captured
            { ok: true, value: answer(verb, payload) }
          rescue Exception => e # rubocop:disable Lint/RescueException — a probe must never end the game
            { ok: false, error: "#{e.class}: #{e.message}", backtrace: (e.backtrace || []).first(8) }
          ensure
            $stdout = previous_stdout
          end
        reply[:stdout] = captured.string
        reply[:ms] = ((Process.clock_gettime(Process::CLOCK_MONOTONIC) - started) * 1000).round(2)
        reply[:frame] = frame
        JSON.generate(reply)
      rescue Exception => e # rubocop:disable Lint/RescueException
        JSON.generate(ok: false, error: "agent runtime: #{e.class}: #{e.message}")
      end

      private

      def answer(verb, payload)
        case verb
        when 'eval' then TOPLEVEL_BINDING.eval(payload, '(agent eval)', 1).inspect
        when 'run' then run_script(*Shellwords.split(payload))
        when 'scripts' then scripts
        else raise ArgumentError, "unknown verb '#{verb}'"
        end
      end

      def run_script(name = nil, *args)
        raise ArgumentError, 'run: no script named (try the `scripts` verb)' if name.nil?

        path = script_paths.map { |dir| File.join(dir, "#{name}.rb") }.find { |file| File.file?(file) }
        raise ArgumentError, "run: no script '#{name}' in #{script_paths.inspect}" unless path

        script_binding(args).eval(File.read(path), path, 1)
      end

      # A fresh scope per run, holding only `args`.
      def script_binding(args)
        binding
      end
    end
  end
end
