require 'minitest/autorun'
require_relative '../build/lib/LiteRGSS'

# A live DisplayWindow is required for Sprite.new — sprites register into
# its DrawableStack at construction time. setup/teardown opens a fresh
# window per test to keep them independent.
class TestSprite < Minitest::Test

  def setup
    @window = LiteRGSS::DisplayWindow.new
  end

  def teardown
    @window.close_window
  end

  # --- initialize ---
  
  def test_initialize_no_viewport
    s = LiteRGSS::Sprite.new
    assert_nil s.viewport
  end
  
  def test_initialize_with_viewport
    vp = LiteRGSS::Viewport.new(0, 0, 640, 480)
    s  = LiteRGSS::Sprite.new(vp)
    assert_equal vp, s.viewport
  end
  
  # --- dispose ---
  
  def test_dispose
    s = LiteRGSS::Sprite.new
    s.dispose
    assert s.disposed?
  end
  
  def test_disposed_raises_on_x
    s = LiteRGSS::Sprite.new
    s.dispose
    assert_raises(RuntimeError) { s.x }
  end
  
  def test_disposed_raises_on_visible
    s = LiteRGSS::Sprite.new
    s.dispose
    assert_raises(RuntimeError) { s.visible }
  end
  
  # --- clone / dup ---
  
  def test_clone_raises
    s = LiteRGSS::Sprite.new
    assert_raises(RuntimeError) { s.clone }
  end
  
  def test_dup_raises
    s = LiteRGSS::Sprite.new
    assert_raises(RuntimeError) { s.dup }
  end
  
  # --- x / y ---
  
  def test_x_default
    s = LiteRGSS::Sprite.new
    assert_equal 0, s.x
  end
  
  def test_x_set
    s = LiteRGSS::Sprite.new
    s.x = 100
    assert_equal 100, s.x
  end
  
  def test_y_default
    s = LiteRGSS::Sprite.new
    assert_equal 0, s.y
  end
  
  def test_y_set
    s = LiteRGSS::Sprite.new
    s.y = 200
    assert_equal 200, s.y
  end
  
  def test_set_position
    s = LiteRGSS::Sprite.new
    s.set_position(50, 75)
    assert_equal 50, s.x
    assert_equal 75, s.y
  end
  
  # --- z ---
  
  def test_z_default
    s = LiteRGSS::Sprite.new
    assert_equal 0, s.z
  end
  
  def test_z_set
    s = LiteRGSS::Sprite.new
    s.z = 5
    assert_equal 5, s.z
  end
  
  # --- ox / oy ---
  
  def test_ox_default
    s = LiteRGSS::Sprite.new
    assert_equal 0, s.ox
  end
  
  def test_ox_set
    s = LiteRGSS::Sprite.new
    s.ox = 16
    assert_equal 16, s.ox
  end
  
  def test_oy_set
    s = LiteRGSS::Sprite.new
    s.oy = 24
    assert_equal 24, s.oy
  end
  
  def test_set_origin
    s = LiteRGSS::Sprite.new
    s.set_origin(8, 12)
    assert_equal 8,  s.ox
    assert_equal 12, s.oy
  end
  
  # --- visible ---
  
  def test_visible_default
    s = LiteRGSS::Sprite.new
    assert s.visible
  end
  
  def test_visible_set_false
    s = LiteRGSS::Sprite.new
    s.visible = false
    refute s.visible
  end
  
  def test_visible_set_true
    s = LiteRGSS::Sprite.new
    s.visible = false
    s.visible = true
    assert s.visible
  end
  
  # --- angle ---
  
  def test_angle_default
    s = LiteRGSS::Sprite.new
    assert_in_delta 0.0, s.angle
  end
  
  def test_angle_set
    s = LiteRGSS::Sprite.new
    s.angle = 45.0
    assert_in_delta 45.0, s.angle
  end
  
  # --- zoom_x / zoom_y / zoom= ---
  
  def test_zoom_x_default
    s = LiteRGSS::Sprite.new
    assert_in_delta 1.0, s.zoom_x
  end
  
  def test_zoom_x_set
    s = LiteRGSS::Sprite.new
    s.zoom_x = 2.0
    assert_in_delta 2.0, s.zoom_x
  end
  
  def test_zoom_y_set
    s = LiteRGSS::Sprite.new
    s.zoom_y = 3.0
    assert_in_delta 3.0, s.zoom_y
  end
  
  def test_zoom_sets_both
    s = LiteRGSS::Sprite.new
    s.zoom = 2.5
    assert_in_delta 2.5, s.zoom_x
    assert_in_delta 2.5, s.zoom_y
  end
  
  # --- opacity ---
  
  def test_opacity_default
    s = LiteRGSS::Sprite.new
    assert_equal 255, s.opacity
  end
  
  def test_opacity_set
    s = LiteRGSS::Sprite.new
    s.opacity = 128
    assert_equal 128, s.opacity
  end
  
  def test_opacity_clamp_high
    s = LiteRGSS::Sprite.new
    s.opacity = 999
    assert_equal 255, s.opacity
  end
  
  def test_opacity_clamp_low
    s = LiteRGSS::Sprite.new
    s.opacity = -10
    assert_equal 0, s.opacity
  end
  
  # --- mirror ---
  
  def test_mirror_default
    s = LiteRGSS::Sprite.new
    refute s.mirror
  end
  
  def test_mirror_set_true
    s = LiteRGSS::Sprite.new
    s.mirror = true
    assert s.mirror
  end
  
  def test_mirror_set_false
    s = LiteRGSS::Sprite.new
    s.mirror = true
    s.mirror = false
    refute s.mirror
  end
  
  # --- src_rect ---
  
  def test_src_rect_default
    s = LiteRGSS::Sprite.new
    assert_equal [0, 0, 0, 0], s.src_rect
  end
  
  def test_src_rect_set
    s = LiteRGSS::Sprite.new
    s.src_rect = [10, 20, 100, 200]
    assert_equal [10, 20, 100, 200], s.src_rect
  end
  
  def test_src_rect_requires_array
    s = LiteRGSS::Sprite.new
    assert_raises(TypeError) { s.src_rect = "bad" }
  end
  
  # --- bitmap ---
  
  def test_bitmap_default_nil
    s = LiteRGSS::Sprite.new
    assert_nil s.bitmap
  end
  
end
