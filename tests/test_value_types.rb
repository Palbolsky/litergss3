require 'minitest/autorun'
require_relative 'litergss'

# Readers return the value as assigned, Integer or Float, like LiteRGSS2:
# games compute with them and pass the result to Integer-only APIs. Forêt
# Éternelle's language menu does `add_text(box.x + box.width / 2, ...)`,
# and Text.new raises "wrong argument type Float (expected Integer)" when
# Sprite#x reads back 131.0 for 131. assert_equal(131, 131.0) passes, so
# these check the class.
class TestValueTypes < Minitest::Test
  def setup
    @window = LiteRGSS::DisplayWindow.new
  end

  def teardown
    @window.close_window
    # Like test_fonts: fonts still loaded when the process exits crash it
    # (double free in the font table's destruction, a bug of its own)
    LiteRGSS::Fonts.clear_all
  end

  def assert_same_value(expected, actual)
    assert_equal expected, actual
    assert_kind_of expected.class, actual
  end

  def test_sprite_defaults_are_integers
    s = LiteRGSS::Sprite.new
    %i[x y ox oy angle zoom_x zoom_y].each do |reader|
      assert_kind_of Integer, s.public_send(reader), "Sprite##{reader}"
    end
  end

  def test_sprite_readers_return_what_was_assigned
    s = LiteRGSS::Sprite.new
    { x: 131, y: 550, ox: 12, oy: 7, angle: 90, zoom_x: 2, zoom_y: 3 }.each do |attr, value|
      s.public_send(:"#{attr}=", value)
      assert_same_value value, s.public_send(attr)
      s.public_send(:"#{attr}=", value + 0.5)
      assert_same_value value + 0.5, s.public_send(attr)
    end
  end

  def test_sprite_set_position_origin_and_zoom
    s = LiteRGSS::Sprite.new
    s.set_position(131, 550)
    assert_same_value 131, s.x
    assert_same_value 550, s.y
    s.set_origin(4, 5)
    assert_same_value 4, s.ox
    assert_same_value 5, s.oy
    s.zoom = 2
    assert_same_value 2, s.zoom_x
    assert_same_value 2, s.zoom_y
    s.zoom = 0.5
    assert_same_value 0.5, s.zoom_x
  end

  # The pattern that failed in Forêt Éternelle
  def test_sprite_arithmetic_feeds_text
    LiteRGSS::Fonts.load_font(0, File.expand_path('assets/Arial.ttf', __dir__))
    vp = LiteRGSS::Viewport.new(0, 0, 640, 480)
    s = LiteRGSS::Sprite.new(vp)
    s.set_position(131, 550)
    x = s.x + s.width / 2
    text = LiteRGSS::Text.new(0, vp, x, s.y + 10, 0, 20, 'Français', 1)
    assert_same_value 131, text.x
  end

  def test_viewport_zoom_and_angle
    vp = LiteRGSS::Viewport.new(0, 0, 640, 480)
    assert_same_value 1, vp.zoom
    vp.zoom = 2
    assert_same_value 2, vp.zoom
    vp.zoom = 1.5
    assert_same_value 1.5, vp.zoom
    vp.angle = 450
    assert_same_value 90, vp.angle
  end

  def test_shape_radius_and_outline
    shape = LiteRGSS::Shape.new(nil, LiteRGSS::Shape::CIRCLE, 10, 16)
    assert_same_value 10, shape.radius
    assert_same_value 0, shape.outline_thickness
    shape.outline_thickness = 2
    assert_same_value 2, shape.outline_thickness
    shape.outline_thickness = 1.5
    assert_same_value 1.5, shape.outline_thickness
  end
end
