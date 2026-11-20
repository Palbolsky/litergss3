# test_image_formats.rb
require 'minitest/autorun'
require_relative 'litergss'

# Images load by their content, not their extension, as on LiteRGSS2 (SFML).
# Lethalmon's graphics/pictures/intro01.png is a JPEG: raylib built without
# its JPEG decoder failed it, and PSDK drew its red "?" placeholder instead.
# The decoders come from raylib's build (tools/deps/build-deps.sh,
# litergss-everywhere's raylib.cmake).
class TestImageFormats < Minitest::Test
  include LiteRGSS

  ASSETS = File.join(__dir__, 'assets')

  def setup
    @window = DisplayWindow.new('Image formats', 160, 120, 1, 32, 60, false, false, true)
  end

  def teardown
    @window&.dispose
  end

  def assert_loads(file, red, green, blue)
    image = Image.new(File.join(ASSETS, file))
    assert_equal [16, 8], [image.width, image.height], file
    color = image.get_pixel(8, 4)
    [[color.red, red], [color.green, green], [color.blue, blue]].each do |got, expected|
      assert_in_delta expected, got, 6, "#{file}: #{color.inspect}"
    end
  ensure
    image&.dispose
  end

  def test_a_jpeg_named_png_loads
    assert_loads('jpeg_named.png', 200, 40, 40)
  end

  def test_a_tga_named_png_loads
    assert_loads('tga_named.png', 40, 40, 200)
  end
end
