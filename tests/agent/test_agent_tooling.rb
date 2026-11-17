# test_agent_tooling.rb — the agent tooling (envs, litergss-run, litergss-agent
# and its scripts), end to end, on a fixture game: the M1 demo of the agent
# map, kept alive. Run by CTest with the build's bin dir as argument.
require 'minitest/autorun'
require 'json'
require 'open3'
require 'fileutils'

class TestAgentTooling < Minitest::Test
  BIN = ARGV.fetch(0) { File.expand_path('../../build/debug/bin', __dir__) }
  GAME = File.expand_path('game', __dir__)
  ENV_NAME = "ctest-#{Process.pid}"

  def tool(*args)
    output, status = Open3.capture2e(File.join(BIN, args.first), *args.drop(1))
    [output, status.exitstatus]
  end

  def agent(*args)
    output, code = tool('litergss-agent', '--env', ENV_NAME, *args)
    [JSON.parse(output.lines.last), code]
  end

  def setup
    output, code = tool('envs', 'up', ENV_NAME, '--game', GAME, '--fresh')
    assert_equal 0, code, output
  end

  def teardown
    tool('envs', 'stop', ENV_NAME, '--timeout', '5')
    tool('envs', 'rm', ENV_NAME)
  end

  def test_launch_see_stop
    output, code = tool('litergss-run', '--env', ENV_NAME, '--no-build', '--timeout', '30')
    assert_equal 0, code, output
    assert_match(/^ready: frame \d+/, output)

    state, code = agent('run', 'state')
    assert_equal 0, code
    assert_equal [160, 120], state.dig('value', 'size')
    assert_operator state.dig('value', 'drawables'), :>=, 1

    shot, code = agent('run', 'shot', 'fixture')
    assert_equal 0, code, shot.inspect
    png = shot.dig('value', 'png')
    assert File.file?(png), png
    assert_includes png, "/.run/#{ENV_NAME}/run-"

    value, code = agent('6 * 7')
    assert_equal 0, code
    assert_equal '42', value['value']

    error, code = agent('raise "boom"')
    assert_equal 1, code
    assert_match(/RuntimeError: boom/, error['error'])

    output, code = tool('envs', 'stop', ENV_NAME)
    assert_equal 0, code, output
    assert_match(/quit sent/, output)
    assert_match(/game exited/, output)
    refute_match(/SIGKILL/, output)
    assert_match(%r{\+ Saves/progress\.txt}, output) # what the run wrote, in the copy
    refute File.exist?(File.join(GAME, 'Saves')), 'the run wrote into the original game folder'
  end

  def test_pause_step_press
    output, code = tool('litergss-run', '--env', ENV_NAME, '--no-build', '--timeout', '30')
    assert_equal 0, code, output
    paused, = agent('run', 'pause')
    held = agent('run', 'state').first.dig('value', 'frame')
    sleep 0.3
    assert_equal held, agent('run', 'state').first.dig('value', 'frame'), 'a held game must not advance'
    assert paused.dig('value', 'paused')

    agent('run', 'press', 'Enter', '3')
    agent('run', 'step', '5')
    assert_equal held + 5, agent('run', 'state').first.dig('value', 'frame'), 'step 5 runs exactly 5 updates'
    enter = agent('LiteRGSS::Inputs::Keyboard::Enter').first['value']
    assert_equal "[[:down, #{enter}], [:up, #{enter}]]", agent('$key_events').first['value']
    assert_equal 3, agent('$polled_enter.size').first['value'].to_i, 'polled state: down for 3 updates'

    agent('run', 'resume')
    output, = tool('envs', 'stop', ENV_NAME)
    assert_match(/game exited/, output)
  end

  def test_stuck_game_is_killed_not_terminated
    output, code = tool('litergss-run', '--env', ENV_NAME, '--no-build', '--timeout', '30', '--', 'hang')
    assert_equal 0, code, output
    sleep 1.5 # past frame 30: the game no longer serves the agent
    output, code = tool('envs', 'stop', ENV_NAME, '--timeout', '2')
    assert_equal 0, code, output
    assert_match(/quit not delivered/, output)
    assert_match(/SIGKILLed/, output)
  end

  def test_scene_runs_and_refuses_a_desktop_window
    preset = File.basename(File.dirname(BIN))
    scene = File.expand_path('../scenes/sprites_viewports.rb', __dir__)
    output, code = tool('litergss-scene', scene, '--on', "preset:#{preset}", '--frames', '2')
    assert_equal 0, code, output
    png = output[%r{(\S+/preset-#{preset}\.png)}, 1]
    assert png && File.file?(png), output

    # LiteRGSS2 has no hidden window: never on the human's display unless asked.
    output, code = tool('litergss-scene', scene, '--on', "preset:#{preset},litergss2")
    assert_equal 2, code, output
    assert_match(/no hidden window/, output)
  end

  def test_unknown_env_is_refused
    output, code = tool('litergss-run', '--env', 'no-such-env', '--no-build')
    assert_equal 1, code
    assert_match(/no env 'no-such-env'/, output)
  end
end
