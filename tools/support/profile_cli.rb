# frozen_string_literal: true

require 'csv'
require 'open3'
require 'optparse'

module LiteRGSSTools
  # litergss-profile — capture N seconds of the Tracy zones of a game an env
  # runs on a Tracy build, and print them as text.
  class ProfileCLI
    USAGE = <<~TXT
      Usage: litergss-profile --env NAME [--seconds N] [--top N] [--json]

      Profiles the game the env runs on this Tracy build (debug-tracy preset:
      build/debug-tracy/bin/litergss-run --env NAME starts it with the profiler on,
      loopback only). Prints frame times and the top zones by total time; the
      trace stays in the run folder (open it in the Tracy GUI for more).
      Needs Tracy's command-line tools (tracy-capture, tracy-csvexport) on PATH or in
      vendor/deps/bin (tools/deps/build-deps.sh --tracy-tools), found at configure time.
    TXT

    def main(argv)
      options = { seconds: 10, top: 10 }
      OptionParser.new do |o|
        o.banner = USAGE
        o.on('--env NAME') { |v| options[:env] = v }
        o.on('--seconds N', Float) { |v| options[:seconds] = v }
        o.on('--top N', Integer) { |v| options[:top] = v }
        o.on('--json') { options[:json] = true }
      end.parse!(argv)
      if options[:env].nil?
        puts USAGE
        return 2
      end
      profile(Env.new(options[:env]), options)
    rescue Error, OptionParser::ParseError => e
      warn "litergss-profile: #{e.message}"
      2
    end

    private

    def tool(name)
      path = BUILD[:"tracy_#{name}"].to_s
      unless File.executable?(path)
        raise Error, "no tracy-#{name} found at configure time (put it on PATH or in vendor/deps/bin: " \
                     'tools/deps/build-deps.sh --tracy-tools; then reconfigure)'
      end

      path
    end

    def profile(env, options)
      raise Error, "build '#{BUILD[:preset]}' is not instrumented: use build/debug-tracy/bin/litergss-profile" unless BUILD[:tracy]

      capture = tool('capture')
      csvexport = tool('csvexport')
      run = env.current
      raise Error, "#{env.name}: no game running (build/#{BUILD[:preset]}/bin/litergss-run --env #{env.name})" unless env.running?
      raise Error, "#{env.name}: its game was not started by this build (#{run['preset']}), so it has no profiler" unless run['tracy_port']

      trace = File.join(run['run_dir'], "profile-#{Time.now.strftime('%H%M%S')}.tracy")
      output, status = Open3.capture2e(capture, '-o', trace, '-a', '127.0.0.1', '-p', run['tracy_port'].to_s, '-f',
                                       '-s', options[:seconds].to_s)
      raise Error, "capture failed: #{output.lines.last(5).join}" unless status.success? && File.file?(trace)

      zones = export(csvexport, trace)
      frames = frame_times(csvexport, trace)
      report(zones, frames, trace, options)
      0
    end

    def export(csvexport, trace)
      output, status = Open3.capture2(csvexport, trace)
      raise Error, "csvexport failed on #{trace}" unless status.success?

      CSV.parse(output, headers: true).map(&:to_h)
    end

    # Frame durations (ms) between the starts of successive DisplayWindow::draw zones.
    def frame_times(csvexport, trace)
      output, status = Open3.capture2(csvexport, '-u', '-f', 'cgss::DisplayWindow::draw', '-c', trace)
      return [] unless status.success?

      starts = CSV.parse(output, headers: true).map { |row| row['ns_since_start'].to_i }.sort
      starts.each_cons(2).map { |a, b| (b - a) / 1e6 }
    end

    def percentile(values, pct)
      return nil if values.empty?

      values.sort[((values.size - 1) * pct).round]
    end

    def report(zones, frames, trace, options)
      top = zones.sort_by { |z| -z['total_ns'].to_i }.first(options[:top])
      if options[:json]
        puts JSON.generate(trace: trace, frames: frames.size + 1, median_ms: percentile(frames, 0.5)&.round(2),
                           p95_ms: percentile(frames, 0.95)&.round(2),
                           zones: top.map { |z| { name: z['name'], calls: z['counts'].to_i, total_ms: (z['total_ns'].to_i / 1e6).round(2), mean_us: (z['mean_ns'].to_i / 1e3).round(1) } })
        return
      end
      if frames.empty?
        puts 'frames: none drawn during the capture'
      else
        puts format('frames %d  median %.1f ms  p95 %.1f ms', frames.size + 1, percentile(frames, 0.5), percentile(frames, 0.95))
      end
      puts format('%-48s %8s %11s %10s', 'zone', 'calls', 'total ms', 'mean us')
      top.each do |z|
        puts format('%-48s %8d %11.1f %10.1f', z['name'][0, 48], z['counts'].to_i, z['total_ns'].to_i / 1e6, z['mean_ns'].to_i / 1e3)
      end
      puts "trace: #{trace}"
    end
  end
end
