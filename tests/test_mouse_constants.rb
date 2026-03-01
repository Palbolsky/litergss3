# test_mouse_constants.rb
require_relative '../build/lib/LiteRGSS'

Mouse = LiteRGSS::Inputs::Mouse

puts "=== Test Mouse Constants ==="

puts "Left        = #{Mouse::Left}"
puts "Right       = #{Mouse::Right}"
puts "Middle      = #{Mouse::Middle}"
puts "XButton1    = #{Mouse::XButton1}"
puts "XButton2    = #{Mouse::XButton2}"
puts "XButton3    = #{Mouse::XButton3}"
puts "VertWheel   = #{Mouse::VerticalWheel}"
puts "HorizWheel  = #{Mouse::HorizontalWheel}"

puts "OK"