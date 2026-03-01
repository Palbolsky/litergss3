# test_window.rb
require 'minitest/autorun'
require_relative '../build/lib/LiteRGSS'

class TestWindow < Minitest::Test
  
  def setup
    @window = LiteRGSS::Window.new
  end
  
  def teardown
    @window.close_window
  end
  
  # --- open_window ---
  
  def test_open_default
    @window.open_window
    refute @window.should_close?, "Window should not close immediately"
  end
  
  def test_open_custom_size
    @window.open_window(800, 600)
    refute @window.should_close?, "Window should not close immediately"
  end
  
  def test_open_custom_size_and_title
    @window.open_window(1280, 720, "My Game")
    refute @window.should_close?, "Window should not close immediately"
  end
  
  def test_open_with_scale
    @window.open_window(320, 240, "Scaled", 2)
    refute @window.should_close?, "Window should not close immediately"
  end
  
  # --- set_title ---
  
  def test_set_title
    @window.open_window
    @window.set_title("New Title")
    refute @window.should_close?
  end
  
  # --- set_icon ---
  
  def test_set_icon_missing_file
    @window.open_window
    assert_raises(RuntimeError) { @window.set_icon("nonexistent.png") }
  end
  
  def test_set_icon_existing_file
    icon_path = File.join(__dir__, "game.png")
    skip "No icon.png found in test directory" unless File.exist?(icon_path)
    @window.open_window
    @window.set_icon(icon_path)
    refute @window.should_close?
  end
  
  # --- set_fps / set_vsync ---
  
  def test_set_fps
    @window.open_window
    @window.set_fps(30)
    refute @window.should_close?
  end
  
  def test_set_vsync_true
    @window.open_window
    @window.set_vsync(true)
    refute @window.should_close?
  end
  
  def test_set_vsync_false
    @window.open_window
    @window.set_vsync(false)
    refute @window.should_close?
  end
  
  # --- scale_window / resize_screen ---
  
  def test_scale_window
    @window.open_window(320, 240)
    @window.scale_window(2)
    refute @window.should_close?
  end
  
  def test_scale_window_back_to_one
    @window.open_window(320, 240)
    @window.scale_window(2)
    @window.scale_window(1)
    refute @window.should_close?
  end
  
  def test_resize_screen
    @window.open_window
    @window.resize_screen(800, 600)
    refute @window.should_close?
  end
  
  # --- move / x / y ---
  
  def test_move
    @window.open_window
    @window.move(100, 100)
    assert_equal 100, @window.x
    assert_equal 100, @window.y
  end
  
  def test_move_origin
    @window.open_window
    @window.move(0, 0)
    assert_equal 0, @window.x
    assert_equal 0, @window.y
  end
  
  # --- desktop_width / desktop_height ---
  
  def test_desktop_width
    @window.open_window
    assert @window.desktop_width > 0, "Desktop width should be > 0"
  end
  
  def test_desktop_height
    @window.open_window
    assert @window.desktop_height > 0, "Desktop height should be > 0"
  end
  
  # --- update ---
  
  def test_update_returns_boolean
    @window.open_window
    result = @window.update
    assert [true, false].include?(result), "update should return true or false"
  end
  
end