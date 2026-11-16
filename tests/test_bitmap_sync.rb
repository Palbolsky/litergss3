# test_bitmap_sync.rb
require 'minitest/autorun'
require_relative 'litergss'

# Edits to a bitmap already shown by drawables reach the screen on the next
# frame, without rebinding it (LiteRGSS2's Bitmap#update contract).
class TestBitmapSync < Minitest::Test
  include LiteRGSS

  def setup
    @window = DisplayWindow.new('Sync', 160, 120, 1, 32, 60, false, false, true)
  end

  def teardown
    @window&.dispose
  end

  def solid(r, g, b)
    img = Image.new(20, 20)
    img.fill_rect(0, 0, 20, 20, Color.new(r, g, b, 255))
    img
  end

  def rgb_at(x, y)
    c = @window.snap_to_bitmap.get_pixel(x, y)
    [c.red, c.green, c.blue]
  end

  def sprite_at(bitmap, x, y)
    Sprite.new(@window).tap { |s| s.bitmap = bitmap; s.set_position(x, y) }
  end

  def test_edit_reaches_bound_sprites
    bitmap = solid(255, 0, 0)
    sprite_at(bitmap, 0, 0)
    sprite_at(bitmap, 40, 0)
    2.times { @window.update }
    assert_equal [255, 0, 0], rgb_at(5, 5)

    bitmap.fill_rect(0, 0, 20, 20, Color.new(0, 255, 0, 255))
    @window.update
    assert_equal [0, 255, 0], rgb_at(5, 5)
    assert_equal [0, 255, 0], rgb_at(45, 5)
  end

  def test_edited_then_disposed_or_collected_bitmaps
    disposed = solid(255, 0, 0)
    sprite_at(disposed, 0, 0)
    @window.update
    disposed.set_pixel(0, 0, Color.new(0, 0, 255, 255))
    disposed.dispose

    5.times do
      img = solid(0, 0, 255)
      sprite_at(img, 60, 0).dispose
      img.set_pixel(1, 1, Color.new(255, 255, 255, 255))
    end
    GC.start
    2.times { @window.update }
    assert_equal [0, 0, 0], rgb_at(70, 5)
  end
end
