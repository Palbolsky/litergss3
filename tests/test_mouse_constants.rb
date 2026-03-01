# test_mouse_constants.rb
require 'minitest/autorun'
require_relative '../build/lib/LiteRGSS'

class TestMouseConstants < Minitest::Test
  Mouse = LiteRGSS::Inputs::Mouse

  EXPECTED_CONSTANTS = %w[
    Left
    Right
    Middle
    XButton1
    XButton2
    XButton3
    VerticalWheel
    HorizontalWheel
  ]

  EXPECTED_CONSTANTS.each do |const|
    define_method("test_#{const.downcase}_exists") do
      assert Mouse.const_defined?(const), "#{const} is not defined"
      assert_kind_of Integer, Mouse.const_get(const), "#{const} should be an Integer"
    end
  end
end