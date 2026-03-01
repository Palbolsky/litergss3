# test_viewport.rb
require 'minitest/autorun'
require_relative '../build/lib/LiteRGSS'

class TestViewport < Minitest::Test
  
  # --- initialize ---
  
  def test_default_initialize
    vp = LiteRGSS::Viewport.new
    refute vp.disposed?
  end
  
  def test_initialize_with_rect
    vp = LiteRGSS::Viewport.new(10, 20, 320, 240)
    assert_equal [10, 20, 320, 240], vp.rect
  end
  
  def test_initialize_partial
    vp = LiteRGSS::Viewport.new(0, 0)
    assert_equal [0, 0, 640, 480], vp.rect
  end
  
  # --- dispose ---
  
  def test_dispose
    vp = LiteRGSS::Viewport.new
    vp.dispose
    assert vp.disposed?
  end
  
  def test_disposed_raises_on_ox
    vp = LiteRGSS::Viewport.new
    vp.dispose
    assert_raises(RuntimeError) { vp.ox }
  end
  
  def test_disposed_raises_on_oy
    vp = LiteRGSS::Viewport.new
    vp.dispose
    assert_raises(RuntimeError) { vp.oy }
  end
  
  def test_disposed_raises_on_visible
    vp = LiteRGSS::Viewport.new
    vp.dispose
    assert_raises(RuntimeError) { vp.visible }
  end
  
  def test_disposed_raises_on_z
    vp = LiteRGSS::Viewport.new
    vp.dispose
    assert_raises(RuntimeError) { vp.z }
  end
  
  # --- clone / dup ---
  
  def test_clone_raises
    vp = LiteRGSS::Viewport.new
    assert_raises(RuntimeError) { vp.clone }
  end
  
  def test_dup_raises
    vp = LiteRGSS::Viewport.new
    assert_raises(RuntimeError) { vp.dup }
  end
  
  # --- ox / oy ---
  
  def test_ox_default
    vp = LiteRGSS::Viewport.new
    assert_equal 0, vp.ox
  end
  
  def test_ox_set
    vp = LiteRGSS::Viewport.new
    vp.ox = 42
    assert_equal 42, vp.ox
  end
  
  def test_oy_default
    vp = LiteRGSS::Viewport.new
    assert_equal 0, vp.oy
  end
  
  def test_oy_set
    vp = LiteRGSS::Viewport.new
    vp.oy = 99
    assert_equal 99, vp.oy
  end
  
  # --- visible ---
  
  def test_visible_default
    vp = LiteRGSS::Viewport.new
    assert vp.visible
  end
  
  def test_visible_set_false
    vp = LiteRGSS::Viewport.new
    vp.visible = false
    refute vp.visible
  end
  
  def test_visible_set_true
    vp = LiteRGSS::Viewport.new
    vp.visible = false
    vp.visible = true
    assert vp.visible
  end
  
  # --- z ---
  
  def test_z_default
    vp = LiteRGSS::Viewport.new
    assert_equal 0, vp.z
  end
  
  def test_z_set
    vp = LiteRGSS::Viewport.new
    vp.z = 5
    assert_equal 5, vp.z
  end
  
  # --- zoom ---
  
  def test_zoom_default
    vp = LiteRGSS::Viewport.new
    assert_in_delta 1.0, vp.zoom
  end
  
  def test_zoom_set
    vp = LiteRGSS::Viewport.new
    vp.zoom = 2.0
    assert_in_delta 2.0, vp.zoom
  end
  
  # --- angle ---
  
  def test_angle_default
    vp = LiteRGSS::Viewport.new
    assert_in_delta 0.0, vp.angle
  end
  
  def test_angle_set
    vp = LiteRGSS::Viewport.new
    vp.angle = 90
    assert_in_delta 90.0, vp.angle
  end
  
  def test_angle_wraps_at_360
    vp = LiteRGSS::Viewport.new
    vp.angle = 400
    assert_in_delta 40.0, vp.angle
  end
  
  # --- rect ---
  
  def test_rect_default
    vp = LiteRGSS::Viewport.new
    assert_equal [0, 0, 640, 480], vp.rect
  end
  
  def test_rect_set
    vp = LiteRGSS::Viewport.new
    vp.rect = [10, 20, 100, 200]
    assert_equal [10, 20, 100, 200], vp.rect
  end
  
  def test_rect_set_requires_array
    vp = LiteRGSS::Viewport.new
    assert_raises(TypeError) { vp.rect = "bad" }
  end
  
end
