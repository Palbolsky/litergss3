# test_mouse_live.rb
# Interactive - move mouse, click buttons, scroll wheel, close window to exit
require_relative '../build/lib/LiteRGSS'

puts "=== Test Mouse Live ==="

LOG_MOUSE_MOVE = true

window = LiteRGSS::Window.new
window.open_window
puts "Window opened"

puts "Position    = #{LiteRGSS::Mouse.position.inspect}"
puts "Press Left  = #{LiteRGSS::Mouse.press?(LiteRGSS::Mouse::Left)}"
puts "Press Right = #{LiteRGSS::Mouse.press?(LiteRGSS::Mouse::Right)}"

LiteRGSS::Mouse.set_position(100, 200)
puts "After set_position(100, 200) = #{LiteRGSS::Mouse.position.inspect}"

MOUSE_BUTTONS = {
"Left"     => LiteRGSS::Mouse::Left,
"Right"    => LiteRGSS::Mouse::Right,
"Middle"   => LiteRGSS::Mouse::Middle,
"XButton1" => LiteRGSS::Mouse::XButton1,
"XButton2" => LiteRGSS::Mouse::XButton2,
}

last_pos      = nil
mouse_pressed = {}

puts "\n[Close the window to exit]"
until window.should_close?
  window.update
  
  pos = LiteRGSS::Mouse.position
  if LOG_MOUSE_MOVE && pos != last_pos
    puts "Mouse moved: #{pos.inspect}"
    last_pos = pos
  end
  
  MOUSE_BUTTONS.each do |name, btn|
    is_down = LiteRGSS::Mouse.press?(btn)
    if is_down && !mouse_pressed[name]
      puts "Mouse pressed:  #{name}"
      mouse_pressed[name] = true
    elsif !is_down && mouse_pressed[name]
      puts "Mouse released: #{name}"
      mouse_pressed[name] = false
    end
  end
  
  wheel = LiteRGSS::Mouse.wheel_move
  puts "Wheel: #{wheel > 0 ? 'up' : 'down'} (#{wheel})" if wheel != 0
end

window.close_window
puts "OK"