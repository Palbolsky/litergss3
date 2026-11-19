# test_font_sizes.rb
require 'minitest/autorun'
require_relative 'litergss'

# A text drawn at another size than its font's: glyphs rasterized at that
# size, as SFML does. The raylib backend rasterized a font at its default
# size only and scaled glyphs to the text's size: nearest sampling doubled
# or dropped pixel columns (a pixel font's "Downloading" read "Dmmnloading").
class TestFontSizes < Minitest::Test
  include LiteRGSS

  FONT = File.expand_path('assets/Arial.ttf', __dir__)
  WIDTH = 200
  ROW = 40

  def setup
    @window = DisplayWindow.new('Font sizes', WIDTH, ROW * 3, 1, 32, 60, false, false, true)
    @viewport = Viewport.new(@window, 0, 0, WIDTH, ROW * 3)
  end

  def teardown
    @window&.dispose
  end

  def load_font(id, size)
    Fonts.load_font(id, FONT)
    Fonts.set_default_size(id, size)
  end

  def text(font_id, row, string, size)
    Text.new(font_id, @viewport, 0, row * ROW, WIDTH, ROW, string, 0).tap do |t|
      t.size = size
      t.fill_color = Color.new(255, 255, 255, 255)
    end
  end

  # The row's pixels, as 0/1 by red channel
  def ink(shot, row)
    (row * ROW...(row + 1) * ROW).map { |y| (0...WIDTH).map { |x| shot.get_pixel(x, y).red > 127 ? 1 : 0 }.join }
  end

  def snapshot
    2.times { @window.update }
    @window.snap_to_bitmap
  end

  def test_a_text_at_another_size_draws_its_glyphs_at_that_size
    load_font(30, 13)
    load_font(31, 22)
    text(30, 0, 'Downloading 42%', 22)
    text(31, 1, 'Downloading 42%', 22)
    shot = snapshot
    refute_empty ink(shot, 1).join.delete('0'), 'nothing drawn'
    assert_equal ink(shot, 1), ink(shot, 0)
  end

  def test_glyphs_added_later_reach_the_other_sizes
    load_font(32, 13)
    load_font(33, 22)
    sized = text(32, 0, 'abc', 22)
    snapshot
    sized.text = 'Œuvre ŝ'
    text(33, 1, 'Œuvre ŝ', 22)
    shot = snapshot
    assert_equal ink(shot, 1), ink(shot, 0)
  end

  def test_a_text_changing_size_follows
    load_font(34, 13)
    load_font(35, 22)
    moving = text(34, 0, 'Size', 22)
    snapshot
    moving.size = 13
    snapshot
    moving.size = 22
    text(35, 1, 'Size', 22)
    shot = snapshot
    assert_equal ink(shot, 1), ink(shot, 0)
  end
end
