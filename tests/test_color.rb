# test_color.rb
require 'minitest/autorun'
require_relative '../build/lib/LiteRGSS'

class TestColor < Minitest::Test
  def test_default_constructor
    c = LiteRGSS::Color.new
    assert_equal 255, c.red
    assert_equal 255, c.green
    assert_equal 255, c.blue
    assert_equal 255, c.alpha
  end

  def test_constructor_with_values
    c = LiteRGSS::Color.new(100, 150, 200, 50)
    assert_equal 100, c.red
    assert_equal 150, c.green
    assert_equal 200, c.blue
    assert_equal 50,  c.alpha
  end

  def test_partial_constructor
    c = LiteRGSS::Color.new(10, 20)
    assert_equal 10,  c.red
    assert_equal 20,  c.green
    assert_equal 255, c.blue
    assert_equal 255, c.alpha
  end

  def test_setters
    c = LiteRGSS::Color.new
    c.red   = 10
    c.green = 20
    c.blue  = 30
    c.alpha = 40
    assert_equal 10, c.red
    assert_equal 20, c.green
    assert_equal 30, c.blue
    assert_equal 40, c.alpha
  end

  def test_set_method
    c = LiteRGSS::Color.new
    c.set(1, 2, 3, 4)
    assert_equal 1, c.red
    assert_equal 2, c.green
    assert_equal 3, c.blue
    assert_equal 4, c.alpha
  end

  def test_clamp_out_of_range
    c = LiteRGSS::Color.new(999, -50, 300, -1)
    assert_equal 255, c.red
    assert_equal 0,   c.green
    assert_equal 255, c.blue
    assert_equal 0,   c.alpha
  end

  def test_equality
    a = LiteRGSS::Color.new(10, 20, 30, 40)
    b = LiteRGSS::Color.new(10, 20, 30, 40)
    c = LiteRGSS::Color.new(1,  2,  3,  4)
    assert_equal a, b
    refute_equal a, c
    assert a.eql?(b)
  end

  def test_copy
    original = LiteRGSS::Color.new(10, 20, 30, 40)
    copy     = original.dup
    assert_equal original.red,   copy.red
    assert_equal original.green, copy.green
    assert_equal original.blue,  copy.blue
    assert_equal original.alpha, copy.alpha
    copy.red = 99
    refute_equal 99, original.red
  end

  def test_serialization
    original = LiteRGSS::Color.new(10, 20, 30, 40)
    dump     = original._dump(0)
    restored = LiteRGSS::Color._load(dump)
    assert_equal original.red,   restored.red
    assert_equal original.green, restored.green
    assert_equal original.blue,  restored.blue
    assert_equal original.alpha, restored.alpha
  end
end