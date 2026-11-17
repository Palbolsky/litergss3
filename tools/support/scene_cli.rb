# frozen_string_literal: true

require 'open3'
require 'optparse'
require_relative 'png'

module LiteRGSSTools
  # litergss-scene — one scenario, several runtimes, one frame each, diffed.
  class SceneCLI
    USAGE = <<~TXT
      Usage: litergss-scene <scenario.rb> [--frames N] [--on LIST] [--diff RUNTIME]
                            [--threshold PCT] [--tolerance N] [--env NAME] [--visible-reference]

      Runs a plain-LiteRGSS scenario (tests/scenes/*.rb) for N frames (default 3)
      under each runtime of --on, saves the last frame of each, and diffs them
      against --diff. Exit 0: every runtime within the threshold; 1: a
      difference over it; 2: could not run.
        --on LIST         runtimes (default raylib,litergss2):
                            raylib, sfml              LiteRGSS3 of build/debug, build/debug-sfml
                            preset:<name>             LiteRGSS3 of build/<name>
                            litergss2                 the LiteRGSS2 reference of this build
                                                      (cmake --build --preset #{BUILD[:preset]} --target litergss2-reference)
        --diff RUNTIME    the reference (default: litergss2 if in --on, else the first)
        --threshold PCT   % of pixels allowed to differ (default 0.1)
        --tolerance N     per-channel difference still counted as equal (default 2)
        --env NAME        use that env's own display (envs up NAME --xvfb)
        --visible-reference  let LiteRGSS2 (no hidden windows) open a window on your display
      Frames land in .run/scenes/<scenario>-<stamp>/: <runtime>.png, <runtime>.diff.png.
    TXT

    PRESET_RUNTIMES = { 'raylib' => 'debug', 'sfml' => 'debug-sfml' }.freeze

    def main(argv)
      options = { frames: 3, on: %w[raylib litergss2], threshold: 0.1, tolerance: 2 }
      OptionParser.new do |o|
        o.banner = USAGE
        o.on('--frames N', Integer) { |v| options[:frames] = v }
        o.on('--on LIST', Array) { |v| options[:on] = v }
        o.on('--diff RUNTIME') { |v| options[:diff] = v }
        o.on('--threshold PCT', Float) { |v| options[:threshold] = v }
        o.on('--tolerance N', Integer) { |v| options[:tolerance] = v }
        o.on('--env NAME') { |v| options[:env] = v }
        o.on('--visible-reference') { options[:visible_reference] = true }
      end.parse!(argv)
      scenario = argv.shift
      if scenario.nil?
        puts USAGE, '', 'Scenarios in tests/scenes/:', Dir.glob(File.join(BUILD[:source_dir], 'tests/scenes/*.rb')).sort.map { |f| "  #{f}" }
        return 2
      end
      raise Error, "no scenario at #{scenario}" unless File.file?(scenario)

      run(File.expand_path(scenario), options)
    rescue Error, OptionParser::ParseError => e
      warn "litergss-scene: #{e.message}"
      2
    end

    private

    def run(scenario, options)
      reference = options[:diff] || (options[:on].include?('litergss2') ? 'litergss2' : options[:on].first)
      raise Error, "--diff #{reference} is not in --on #{options[:on].join(',')}" unless options[:on].include?(reference)

      display = display_for(options)
      out_dir = File.join(LiteRGSSTools.run_root, 'scenes', "#{File.basename(scenario, '.rb')}-#{Time.now.strftime('%Y%m%d-%H%M%S')}-#{Process.pid}")
      FileUtils.mkdir_p(out_dir)
      shots = options[:on].to_h { |runtime| [runtime, shoot(runtime, scenario, options[:frames], out_dir, display)] }
      shots.each { |runtime, shot| puts format('%-10s %s  frame %d  %dx%d%s', runtime, shot[:png], options[:frames], *shot[:size], runtime == reference ? '  (reference)' : '') }

      ref = PNG.read(shots[reference][:png])
      failed = false
      shots.each_key do |runtime|
        next if runtime == reference

        result = compare(PNG.read(shots[runtime][:png]), ref, options[:tolerance], File.join(out_dir, "#{runtime}.diff.png"))
        over = result[:percent] > options[:threshold]
        failed ||= over
        detail = result[:bbox] ? " (bbox #{result[:bbox].join(',')}) → #{result[:diff_png]}" : ''
        puts format('%s vs %s: %.2f%% pixels differ%s   %s', runtime, reference, result[:percent], detail,
                    over ? "FAIL (threshold #{options[:threshold]}%)" : 'ok')
      end
      failed ? 1 : 0
    end

    def display_for(options)
      if options[:env]
        env = Env.new(options[:env])
        raise Error, "no env '#{options[:env]}' (envs up #{options[:env]} --xvfb)" unless env.exist?
        return env.display_env if env.display
      end
      if options[:on].include?('litergss2') && !options[:visible_reference]
        raise Error, 'LiteRGSS2 has no hidden window: give it a display of its own (envs up NAME --xvfb, then --env NAME), ' \
                     'or allow a window on yours (--visible-reference), or leave it out (--on raylib,sfml)'
      end
      nil
    end

    def runtime_lib(runtime)
      if runtime == 'litergss2'
        lib = File.join(BUILD[:reference_dir], 'lib', 'LiteRGSS')
        raise Error, "no LiteRGSS2 reference built (cmake --build --preset #{BUILD[:preset]} --target litergss2-reference)" unless File.exist?("#{lib}.so")

        return lib
      end
      preset = runtime.start_with?('preset:') ? runtime.delete_prefix('preset:') : PRESET_RUNTIMES[runtime]
      raise Error, "unknown runtime '#{runtime}' (raylib, sfml, preset:<name>, litergss2)" unless preset

      lib = File.join(BUILD[:source_dir], 'build', preset, 'lib', 'LiteRGSS')
      raise Error, "runtime #{runtime}: no #{lib}.so (cmake --build --preset #{preset})" unless File.exist?("#{lib}.so")

      lib
    end

    def shoot(runtime, scenario, frames, out_dir, display)
      lib = runtime_lib(runtime)
      png = File.join(out_dir, "#{runtime.tr(':', '-')}.png")
      env = { 'LITECGSS_LOG_DIR' => FileUtils.mkdir_p(File.join(out_dir, "logs-#{runtime.tr(':', '-')}")).first }
      env['LITERGSS_HIDDEN_WINDOW'] = '1' unless display
      env['DISPLAY'] = display if display
      env['LITECGSS_NO_VSYNC'] = '1' if display
      output, status = Open3.capture2e(env, BUILD[:ruby], File.join(__dir__, 'scene_runner.rb'), lib, scenario, frames.to_s, png,
                                       chdir: out_dir)
      File.write(File.join(out_dir, "#{runtime.tr(':', '-')}.log"), output)
      unless status.success? && File.file?(png)
        raise Error, "#{runtime} failed (exit #{status.exitstatus}): #{output.lines.reject { |l| l.start_with?('INFO:') }.last(8).join}"
      end

      JSON.parse(output.lines.reverse.find { |l| l.start_with?('{') })
          .then { |r| { png: png, size: r['size'] } }
    end

    # Pixels whose channels differ by more than `tolerance`: share, bounding
    # box, and a diff image (the reference dimmed, differing pixels red).
    def compare(image, ref, tolerance, diff_path)
      unless image.width == ref.width && image.height == ref.height
        return { percent: 100.0, bbox: [0, 0, image.width, image.height], diff_png: '(sizes differ)' }
      end

      a = image.rgba.bytes
      b = ref.rgba.bytes
      diff = String.new(capacity: a.size, encoding: Encoding::BINARY)
      count = 0
      min_x = min_y = Float::INFINITY
      max_x = max_y = -1
      (0...(a.size / 4)).each do |i|
        o = i * 4
        differs = (0..3).any? { |c| (a[o + c] - b[o + c]).abs > tolerance }
        if differs
          count += 1
          x = i % ref.width
          y = i / ref.width
          min_x = x if x < min_x
          max_x = x if x > max_x
          min_y = y if y < min_y
          max_y = y if y > max_y
          diff << [255, 0, 0, 255].pack('C4')
        else
          diff << [b[o] / 4, b[o + 1] / 4, b[o + 2] / 4, 255].pack('C4')
        end
      end
      return { percent: 0.0 } if count.zero?

      PNG.write(diff_path, ref.width, ref.height, diff)
      { percent: 100.0 * count / (a.size / 4), bbox: [min_x, min_y, max_x, max_y], diff_png: diff_path }
    end
  end
end
