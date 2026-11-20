# test_font_missing_glyphs.rb
require 'minitest/autorun'
require_relative 'litergss'

# A character the font lacks is drawn as the font's own missing glyph, as
# FreeType draws it for SFML. raylib drew '?' instead: Lethalmon's splash read
# "? Initializing… ?" (its PokemonDS has no ‹ ›), its dialogs got stray '?'.
class TestFontMissingGlyphs < Minitest::Test
  include LiteRGSS

  FONT = File.expand_path('assets/Arial.ttf', __dir__)
  WIDTH = 60
  ROW = 30

  def setup
    @window = DisplayWindow.new('Missing glyphs', WIDTH, ROW * 3, 1, 32, 60, false, false, true)
    @viewport = Viewport.new(@window, 0, 0, WIDTH, ROW * 3)
    Fonts.load_font(40, FONT)
    Fonts.set_default_size(40, 20)
  end

  def teardown
    @window&.dispose
  end

  def text(row, string)
    Text.new(40, @viewport, 0, row * ROW, WIDTH, ROW, string, 0).tap { |t| t.fill_color = Color.new(255, 255, 255, 255) }
  end

  def ink(shot, row)
    (row * ROW...(row + 1) * ROW).map { |y| (0...WIDTH).map { |x| shot.get_pixel(x, y).red > 127 ? 1 : 0 }.join }
  end

  def test_a_character_the_font_lacks_is_its_missing_glyph_not_a_question_mark
    text(0, "\u{E000}") # private use: no font has it
    text(1, "\u{E001}")
    text(2, '?')
    2.times { @window.update }
    shot = @window.snap_to_bitmap
    assert_equal ink(shot, 0), ink(shot, 1), 'two missing characters: the same glyph'
    refute_equal ink(shot, 2), ink(shot, 0), "drawn as '?'"
  end
end
