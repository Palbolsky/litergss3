# test_keyboard_constants.rb
require_relative '../build/lib/LiteRGSS'

Keyboard = LiteRGSS::Inputs::Keyboard

puts "=== Test Keyboard Constants ==="

puts "A        = #{Keyboard::A}"
puts "Escape   = #{Keyboard::Escape}"
puts "Enter    = #{Keyboard::Enter}"
puts "Space    = #{Keyboard::Space}"
puts "Left     = #{Keyboard::Left}"
puts "Right    = #{Keyboard::Right}"
puts "Up       = #{Keyboard::Up}"
puts "Down     = #{Keyboard::Down}"
puts "LShift   = #{Keyboard::LShift}"
puts "LControl = #{Keyboard::LControl}"

puts "OK"