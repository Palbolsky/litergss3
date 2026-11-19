# test_image_gpu_pixels.rb
require 'minitest/autorun'
require_relative 'litergss'

# An image no script touched since it was created or loaded keeps its pixels
# on the GPU only, once uploaded (LiteRGSS2's Bitmap was GPU-only): big
# pictures no longer live twice. Reading or editing it reads them back once.
class TestImageGpuPixels < Minitest::Test
  include LiteRGSS

  ASSET = File.expand_path('assets/game.png', __dir__)

  def setup
    @window = DisplayWindow.new('GPU pixels', 160, 120, 1, 32, 60, false, false, true)
  end

  def teardown
    @window&.dispose
  end

  def show(image)
    Sprite.new(@window).tap { |s| s.bitmap = image }
    2.times { @window.update }
    image
  end

  def rgba(color)
    [color.red, color.green, color.blue, color.alpha]
  end

  def screen(x, y)
    rgba(@window.snap_to_bitmap.get_pixel(x, y))[0, 3]
  end

  def test_a_loaded_image_shown_keeps_its_pixels_on_the_gpu_only
    image = show(Image.new(ASSET))
    refute image.__pixels_resident?
    assert_equal Image.new(ASSET).then { |i| [i.width, i.height] }, [image.width, image.height]
    refute image.__pixels_resident?, 'its size needs no pixels'
  end

  def test_released_pixels_read_back_unchanged
    reference = Image.new(ASSET)
    image = show(Image.new(ASSET))
    [[0, 0], [reference.width / 2, reference.height / 2], [reference.width - 1, reference.height - 1]].each do |x, y|
      assert_equal rgba(reference.get_pixel(x, y)), rgba(image.get_pixel(x, y)), "pixel #{x},#{y}"
    end
    assert image.__pixels_resident?, 'read back once, then kept'
  end

  def test_an_edit_after_the_release_reaches_the_screen
    image = show(Image.new(20, 20))
    refute image.__pixels_resident?
    image.fill_rect(0, 0, 20, 20, Color.new(0, 255, 0, 255))
    @window.update
    assert_equal [0, 255, 0], screen(5, 5)
  end

  def test_an_image_edited_before_it_is_shown_keeps_its_pixels
    image = Image.new(20, 20)
    image.fill_rect(0, 0, 20, 20, Color.new(255, 0, 0, 255))
    show(image)
    assert image.__pixels_resident?
    assert_equal [255, 0, 0], screen(5, 5)
  end

  def test_a_copy_of_a_released_image_has_its_pixels
    image = show(Image.new(ASSET))
    copy = image.dup
    assert_equal rgba(Image.new(ASSET).get_pixel(3, 3)), rgba(copy.get_pixel(3, 3))
  end

  def test_a_released_image_is_a_blit_source
    image = show(Image.new(20, 20).tap { |i| i.fill_rect(0, 0, 20, 20, Color.new(0, 0, 255, 255)) })
    target = Image.new(20, 20)
    target.blt(0, 0, Image.new(ASSET).then { |a| show(a) }, Rect.new(0, 0, 10, 10))
    assert_equal rgba(Image.new(ASSET).get_pixel(2, 2)), rgba(target.get_pixel(2, 2))
    assert image.__pixels_resident?
  end

  def test_a_snapshot_keeps_its_pixels_on_the_gpu_until_read
    viewport = Viewport.new(@window, 0, 0, 160, 120)
    Sprite.new(viewport).tap { |s| s.bitmap = Image.new(20, 20).tap { |i| i.fill_rect(0, 0, 20, 20, Color.new(255, 0, 0, 255)) } }
    2.times { @window.update }
    shot = viewport.snap_to_bitmap
    refute shot.__pixels_resident?
    assert_equal [255, 0, 0, 255], rgba(shot.get_pixel(5, 5))
    assert shot.__pixels_resident?
  end
end
