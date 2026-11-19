# test_image_memory.rb
require 'minitest/autorun'
require_relative 'litergss'

# Pixels go back when an image or a bitmap goes. On raylib the native image
# is a C struct owning its buffer: dropped without being unloaded (a
# defaulted destructor or move), each one leaked its pixels, ~800 MB per
# trainer battle in Forêt Éternelle on Android.
class TestImageMemory < Minitest::Test
  include LiteRGSS

  COUNT = 200 # 1280x720 RGBA: ~3.7 MB each, ~740 MB if they leak
  MAX_GROWTH_KB = 150_000

  def setup
    @window = DisplayWindow.new('Image memory', 160, 120, 1, 32, 60, false, false, true)
  end

  def teardown
    @window&.dispose
  end

  def rss_kb
    File.read('/proc/self/status')[/VmRSS:\s+(\d+)/, 1].to_i
  end

  def assert_no_growth(label)
    skip 'needs /proc' unless File.exist?('/proc/self/status')
    3.times { yield } # allocator warm-up
    GC.start
    before = rss_kb
    # Collected every ten: the memory a collection frees is reused by the next
    # ones (an allocator doesn't give it back to the system, all at once)
    COUNT.times { |i| yield; GC.start if i % 10 == 9 }
    GC.start
    growth = rss_kb - before
    assert_operator growth, :<, MAX_GROWTH_KB, "#{label}: RSS grew by #{growth} kB"
  end

  def test_disposed_images_free_their_pixels
    assert_no_growth('Image#dispose') { Image.new(1280, 720).dispose }
  end

  def test_disposed_bitmaps_free_their_pixels
    assert_no_growth('Bitmap#dispose') { Bitmap.new(1280, 720).dispose }
  end

  def test_collected_bitmaps_free_their_pixels
    assert_no_growth('Bitmap collected') { Bitmap.new(1280, 720) }
  end
end
