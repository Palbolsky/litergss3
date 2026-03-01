# test_gamepad_constants.rb
require_relative '../build/lib/LiteRGSS'

Gamepad = LiteRGSS::Inputs::Gamepad

puts "=== Test Gamepad Constants ==="

puts "BUTTON_LEFT_FACE_DOWN   = #{Gamepad::BUTTON_LEFT_FACE_DOWN}"
puts "BUTTON_LEFT_FACE_LEFT   = #{Gamepad::BUTTON_LEFT_FACE_LEFT}"
puts "BUTTON_LEFT_FACE_RIGHT  = #{Gamepad::BUTTON_LEFT_FACE_RIGHT}"
puts "BUTTON_LEFT_FACE_UP     = #{Gamepad::BUTTON_LEFT_FACE_UP}"
puts "BUTTON_LEFT_THUMB       = #{Gamepad::BUTTON_LEFT_THUMB}"
puts "BUTTON_LEFT_TRIGGER_1   = #{Gamepad::BUTTON_LEFT_TRIGGER_1}"
puts "BUTTON_LEFT_TRIGGER_2   = #{Gamepad::BUTTON_LEFT_TRIGGER_2}"
puts "BUTTON_MIDDLE           = #{Gamepad::BUTTON_MIDDLE}"
puts "BUTTON_MIDDLE_LEFT      = #{Gamepad::BUTTON_MIDDLE_LEFT}"
puts "BUTTON_MIDDLE_RIGHT     = #{Gamepad::BUTTON_MIDDLE_RIGHT}"
puts "BUTTON_RIGHT_FACE_DOWN  = #{Gamepad::BUTTON_RIGHT_FACE_DOWN}"
puts "BUTTON_RIGHT_FACE_LEFT  = #{Gamepad::BUTTON_RIGHT_FACE_LEFT}"
puts "BUTTON_RIGHT_FACE_RIGHT = #{Gamepad::BUTTON_RIGHT_FACE_RIGHT}"
puts "BUTTON_RIGHT_FACE_UP    = #{Gamepad::BUTTON_RIGHT_FACE_UP}"
puts "BUTTON_RIGHT_THUMB      = #{Gamepad::BUTTON_RIGHT_THUMB}"
puts "BUTTON_RIGHT_TRIGGER_1  = #{Gamepad::BUTTON_RIGHT_TRIGGER_1}"
puts "BUTTON_RIGHT_TRIGGER_2  = #{Gamepad::BUTTON_RIGHT_TRIGGER_2}"
puts "BUTTON_UNKNOWN          = #{Gamepad::BUTTON_UNKNOWN}"
puts "AXIS_LEFT_TRIGGER       = #{Gamepad::AXIS_LEFT_TRIGGER}"
puts "AXIS_LEFT_X             = #{Gamepad::AXIS_LEFT_X}"
puts "AXIS_LEFT_Y             = #{Gamepad::AXIS_LEFT_Y}"
puts "AXIS_RIGHT_TRIGGER      = #{Gamepad::AXIS_RIGHT_TRIGGER}"
puts "AXIS_RIGHT_X            = #{Gamepad::AXIS_RIGHT_X}"
puts "AXIS_RIGHT_Y            = #{Gamepad::AXIS_RIGHT_Y}"

puts "OK"