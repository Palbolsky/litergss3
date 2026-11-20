# test_yuki_gif.rb
require 'minitest/autorun'
require_relative 'litergss'

# Yuki::GifReader draws its frames on a Bitmap: PSDK's animated battlers
# (Lethalmon) do so every frame. LiteRGSS3 raised NotImplementedError, and the
# game ended on its first battle.
class TestYukiGif < Minitest::Test
  include LiteRGSS

  GIF = File.expand_path('assets/two_frames.gif', __dir__) # 8x6: red, then blue (100 ms)

  def setup
    @window = DisplayWindow.new('GIF', 64, 64, 1, 32, 60, false, false, true)
  end

  def teardown
    Yuki::GifReader.delta_counter = 1000 / 60.0
    @window&.dispose
  end

  def rgb(image, x, y)
    c = image.get_pixel(x, y)
    [c.red, c.green, c.blue]
  end

  def test_draw_puts_the_frame_on_the_bitmap
    gif = Yuki::GifReader.new(GIF)
    bitmap = Image.new(gif.width, gif.height)
    gif.update(bitmap) # the first update decodes frame 0
    assert_equal [255, 0, 0], rgb(bitmap, 3, 3)
  end

  def test_update_moves_to_the_next_frame_as_time_passes
    gif = Yuki::GifReader.new(File.binread(GIF), true)
    bitmap = Image.new(gif.width, gif.height)
    Yuki::GifReader.delta_counter = 60.0 # ms a frame
    gif.update(bitmap)
    assert_equal 0, gif.frame
    assert_equal [255, 0, 0], rgb(bitmap, 0, 0)
    3.times { gif.update(bitmap) }
    assert_equal 1, gif.frame
    assert_equal [0, 0, 255], rgb(bitmap, 0, 0)
  end

  # A script edits the bitmap, then a frame is drawn: what it reads is the frame
  def test_the_frame_replaces_edited_pixels
    gif = Yuki::GifReader.new(GIF)
    bitmap = Image.new(gif.width, gif.height)
    bitmap.fill_rect(0, 0, gif.width, gif.height, Color.new(0, 255, 0, 255))
    gif.update(bitmap)
    assert_equal [255, 0, 0], rgb(bitmap, 1, 1)
  end

  def test_a_bitmap_smaller_than_the_gif_is_refused
    gif = Yuki::GifReader.new(GIF)
    assert_raises(ArgumentError) { gif.draw(Image.new(4, 4)) }
  end

  # PSDK's error log mixes exception messages into UTF-8 text: an engine
  # message must be valid there (a non-ASCII one, binary, raised there)
  def test_engine_messages_mix_with_utf8_text
    error = assert_raises(ArgumentError) { Yuki::GifReader.new(GIF).draw(Image.new(4, 4)) }
    assert_kind_of String, "Erreur de script : #{error.message} é"
  end
end
