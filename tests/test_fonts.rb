# test_fonts.rb
require 'minitest/autorun'
require_relative '../build/lib/LiteRGSS'

FONT_PATH = File.expand_path('./assets/Arial.ttf', __dir__)
Fonts = LiteRGSS::Fonts;
Color = LiteRGSS::Color;

class TestFonts < Minitest::Test
  def setup
    Fonts.clear_all
  end

  def teardown
    Fonts.clear_all
  end

  # --- load_font ---
  def test_load_font
    skip "Font file not found: #{FONT_PATH}" unless File.exist?(FONT_PATH)
    Fonts.load_font(0, FONT_PATH)
  end

  # --- default size ---
  def test_set_and_get_default_size
    Fonts.set_default_size(0, 24)
    assert_equal 24, Fonts.get_default_size(0)
  end

  def test_set_default_size_multiple_fonts
    Fonts.set_default_size(0, 12)
    Fonts.set_default_size(1, 32)
    assert_equal 12, Fonts.get_default_size(0)
    assert_equal 32, Fonts.get_default_size(1)
  end

  # --- fill color ---
  def test_set_and_get_fill_color
    color = Color.new(255, 0, 0, 255)
    Fonts.define_fill_color(0, color)
    result = Fonts.get_fill_color(0)
    assert_equal color, result
  end

  # --- outline color ---
  def test_set_and_get_outline_color
    color = Color.new(0, 255, 0, 255)
    Fonts.define_outline_color(0, color)
    result = Fonts.get_outline_color(0)
    assert_equal color, result
  end

  # --- shadow color ---
  def test_set_and_get_shadow_color
    color = Color.new(0, 0, 255, 255)
    Fonts.define_shadow_color(0, color)
    result = Fonts.get_shadow_color(0)
    assert_equal color, result
  end

  # --- instance variables exist ---
  def test_instance_variables_exist
    color = Color.new(255, 0, 0, 255)
    Fonts.set_default_size(0, 24)
    Fonts.define_fill_color(0, color)
    Fonts.define_outline_color(0, color)
    Fonts.define_shadow_color(0, color)

    assert Fonts.instance_variable_defined?(:@default_size), "@default_size not defined"
    assert Fonts.instance_variable_defined?(:@fill_color), "@fill_color not defined"
    assert Fonts.instance_variable_defined?(:@outline_color), "@outline_color not defined"
    assert Fonts.instance_variable_defined?(:@shadow_color), "@shadow_color not defined"
  end

  # --- clear_all ---
  def test_clear_all
    Fonts.set_default_size(0, 24)
    Fonts.clear_all
    assert_equal 16, Fonts.get_default_size(0)
  end
end