# test_gamepad_constants.rb
require 'minitest/autorun'
require_relative '../build/lib/LiteRGSS'

class TestGamepadConstants < Minitest::Test
  Gamepad = LiteRGSS::Inputs::Gamepad

  EXPECTED_CONSTANTS = %w[
    BUTTON_LEFT_FACE_DOWN
    BUTTON_LEFT_FACE_LEFT
    BUTTON_LEFT_FACE_RIGHT
    BUTTON_LEFT_FACE_UP
    BUTTON_LEFT_THUMB
    BUTTON_LEFT_TRIGGER_1
    BUTTON_LEFT_TRIGGER_2
    BUTTON_MIDDLE
    BUTTON_MIDDLE_LEFT
    BUTTON_MIDDLE_RIGHT
    BUTTON_RIGHT_FACE_DOWN
    BUTTON_RIGHT_FACE_LEFT
    BUTTON_RIGHT_FACE_RIGHT
    BUTTON_RIGHT_FACE_UP
    BUTTON_RIGHT_THUMB
    BUTTON_RIGHT_TRIGGER_1
    BUTTON_RIGHT_TRIGGER_2
    BUTTON_UNKNOWN
    AXIS_LEFT_TRIGGER
    AXIS_LEFT_X
    AXIS_LEFT_Y
    AXIS_RIGHT_TRIGGER
    AXIS_RIGHT_X
    AXIS_RIGHT_Y
  ]

  EXPECTED_CONSTANTS.each do |const|
    define_method("test_#{const.downcase}_exists") do
      assert Gamepad.const_defined?(const), "#{const} is not defined"
      assert_kind_of Integer, Gamepad.const_get(const), "#{const} should be an Integer"
    end
  end
end