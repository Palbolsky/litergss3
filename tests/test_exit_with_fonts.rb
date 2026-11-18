require 'minitest/autorun'
require 'rbconfig'

# Fonts still loaded when the process exits are destroyed with LiteRGSS's
# font table (a C++ static), after Ruby and the window: their atlas registry
# (another file's static) could be destroyed first (double free), and the
# GL calls freeing their texture crashed (the GL library may be gone). A game
# quitting with its fonts loaded must exit cleanly. In a child process: the
# crash is at exit.
class TestExitWithFonts < Minitest::Test
  FONT = File.expand_path('assets/Arial.ttf', __dir__)

  def run_child(close_window)
    script = <<~RUBY
      require #{File.join(__dir__, 'litergss').inspect}
      window = LiteRGSS::DisplayWindow.new
      LiteRGSS::Fonts.load_font(0, #{FONT.inspect})
      viewport = LiteRGSS::Viewport.new(0, 0, 640, 480)
      LiteRGSS::Text.new(0, viewport, 10, 10, 0, 20, 'Fin', 1)
      window.close_window if #{close_window}
    RUBY
    _out, err, status = Open3.capture3(RbConfig.ruby, '-e', script)
    [status, err]
  end

  def setup
    require 'open3'
  end

  def test_exit_with_fonts_loaded_and_the_window_open
    status, err = run_child(false)
    assert status.success?, "exit status #{status.inspect}: #{err.lines.last(3).join}"
  end

  def test_exit_with_fonts_loaded_after_closing_the_window
    status, err = run_child(true)
    assert status.success?, "exit status #{status.inspect}: #{err.lines.last(3).join}"
  end
end
