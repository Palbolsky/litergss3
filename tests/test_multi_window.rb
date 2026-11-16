# test_multi_window.rb
require 'minitest/autorun'
require_relative 'litergss'

# Several DisplayWindows, as PSDK's error screen opens one from a thread of
# its own while the game's keeps updating. Backends with one OS window
# (raylib) stack them: the last opened owns the screen until it closes.
class TestMultiWindow < Minitest::Test
  include LiteRGSS

  def solid(r, g, b)
    img = Image.new(20, 20)
    img.fill_rect(0, 0, 20, 20, Color.new(r, g, b, 255))
    img
  end

  def rgb(img, x, y)
    c = img.get_pixel(x, y)
    [c.red, c.green, c.blue]
  end

  def test_second_window_from_a_thread
    game = DisplayWindow.new('Game', 320, 240, 1, 32, 60, false, false, true)
    Sprite.new(game).tap { |s| s.bitmap = solid(0, 0, 255); s.set_position(5, 5) }
    3.times { game.update }
    assert_equal [0, 0, 255], rgb(game.snap_to_bitmap, 10, 10)

    running = true
    seen = {}
    thread = Thread.new do
      error = DisplayWindow.new('Error', 480, 200, 1, 32, 20, false, false, false)
      Sprite.new(error).tap { |s| s.bitmap = solid(255, 0, 0); s.set_position(5, 5) }
      Sprite.new(Viewport.new(error, 100, 0, 200, 100)).tap { |s| s.bitmap = solid(0, 255, 0); s.set_position(5, 5) }
      10.times { error.update; sleep 0.02 }
      shot = error.snap_to_bitmap
      seen = { size: [shot.width, shot.height], window: rgb(shot, 10, 10), viewport: rgb(shot, 110, 10) }
      error.dispose
    ensure
      running = false
    end
    while running
      game.update
      sleep 0.05
    end
    thread.join

    assert_equal({ size: [480, 200], window: [255, 0, 0], viewport: [0, 255, 0] }, seen)
    3.times { game.update }
    shot = game.snap_to_bitmap
    assert_equal [320, 240], [shot.width, shot.height]
    assert_equal [0, 0, 255], rgb(shot, 10, 10)
    refute game.should_close?
  ensure
    game&.dispose
  end
end
