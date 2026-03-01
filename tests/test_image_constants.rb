require 'minitest/autorun'
require_relative '../build/lib/LiteRGSS'

class TestImage < Minitest::Test
  
  SAMPLE_IMAGE = File.join(__dir__, "game.png")
  
  # --- initialize ---
  
  def test_initialize_empty
    img = LiteRGSS::Image.new(64, 64)
    assert_equal 64, img.width
    assert_equal 64, img.height
    refute img.disposed?
  end
  
  def test_initialize_from_file
    skip "No sample.png found" unless File.exist?(SAMPLE_IMAGE)
    img = LiteRGSS::Image.new(SAMPLE_IMAGE)
    assert img.width  > 0
    assert img.height > 0
  end
  
  def test_initialize_invalid_file
    assert_raises(RuntimeError) { LiteRGSS::Image.new("nonexistent.png") }
  end
  
  def test_initialize_invalid_size
    assert_raises(RuntimeError) { LiteRGSS::Image.new(0, 0) }
  end
  
  def test_initialize_negative_size
    assert_raises(RuntimeError) { LiteRGSS::Image.new(-1, 10) }
  end
  
  # --- dispose ---
  
  def test_dispose
    img = LiteRGSS::Image.new(32, 32)
    img.dispose
    assert img.disposed?
  end
  
  def test_disposed_raises_on_width
    img = LiteRGSS::Image.new(32, 32)
    img.dispose
    assert_raises(RuntimeError) { img.width }
  end
  
  def test_disposed_raises_on_height
    img = LiteRGSS::Image.new(32, 32)
    img.dispose
    assert_raises(RuntimeError) { img.height }
  end
  
  # --- copy ---
  
  def test_initialize_copy
    img  = LiteRGSS::Image.new(32, 32)
    copy = img.dup
    assert_equal img.width,  copy.width
    assert_equal img.height, copy.height
  end
  
  # --- rect ---
  
  def test_rect
    img = LiteRGSS::Image.new(100, 200)
    assert_equal [0, 0, 100, 200], img.rect
  end
  
  # --- get_pixel / set_pixel ---
  
  def test_set_and_get_pixel
    img   = LiteRGSS::Image.new(32, 32)
    color = LiteRGSS::Color.new(255, 0, 0, 255)
    img.set_pixel(10, 10, color)
    result = img.get_pixel(10, 10)
    assert_equal 255, result.red
    assert_equal 0,   result.green
    assert_equal 0,   result.blue
    assert_equal 255, result.alpha
  end
  
  def test_get_pixel_out_of_bounds
    img = LiteRGSS::Image.new(32, 32)
    assert_nil img.get_pixel(100, 100)
  end
  
  def test_get_pixel_alpha
    img   = LiteRGSS::Image.new(32, 32)
    color = LiteRGSS::Color.new(0, 0, 0, 128)
    img.set_pixel(5, 5, color)
    assert_equal 128, img.get_pixel_alpha(5, 5)
  end
  
  # --- fill_rect ---
  
  def test_fill_rect
    img   = LiteRGSS::Image.new(32, 32)
    color = LiteRGSS::Color.new(0, 255, 0, 255)
    img.fill_rect(0, 0, 10, 10, color)
    result = img.get_pixel(5, 5)
    assert_equal 0,   result.red
    assert_equal 255, result.green
  end
  
  # --- clear_rect ---
  
  def test_clear_rect
    img   = LiteRGSS::Image.new(32, 32)
    color = LiteRGSS::Color.new(255, 0, 0, 255)
    img.fill_rect(0, 0, 32, 32, color)
    img.clear_rect(0, 0, 32, 32)
    result = img.get_pixel(5, 5)
    assert_equal 0, result.alpha
  end
  
  # --- blt ---
  
  def test_blt
    src = LiteRGSS::Image.new(32, 32)
    dst = LiteRGSS::Image.new(64, 64)
    color = LiteRGSS::Color.new(0, 0, 255, 255)
    src.fill_rect(0, 0, 32, 32, color)
    dst.blt(0, 0, src, [0, 0, 32, 32])
    result = dst.get_pixel(10, 10)
    assert_equal 0,   result.red
    assert_equal 0,   result.green
    assert_equal 255, result.blue
  end
  
  # --- to_png ---
  
  def test_to_png_returns_string
    img  = LiteRGSS::Image.new(32, 32)
    data = img.to_png
    assert_kind_of String, data
    assert data.length > 0
  end
  
  # --- to_png_file ---
  
  def test_to_png_file
    img  = LiteRGSS::Image.new(32, 32)
    path = File.join(__dir__, "output_test.png")
    result = img.to_png_file(path)
    assert result
    assert File.exist?(path)
  ensure
    File.delete(path) if File.exist?(path)
  end
  
end
