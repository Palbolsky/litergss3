# test_mouse_live.rb
# Interactive - move mouse, click buttons, scroll wheel, close window to exit
require_relative '../build/lib/LiteRGSS'

Mouse = LiteRGSS::Inputs::Mouse

puts "=== Test Mouse Live ==="

LOG_MOUSE_MOVE = true

window = LiteRGSS::Window.new
window.open_window
puts "Window opened"

puts "Position    = #{Mouse.position.inspect}"
puts "Press Left  = #{Mouse.press?(Mouse::Left)}"
puts "Press Right = #{Mouse.press?(Mouse::Right)}"

Mouse.set_position(100, 200)
puts "After set_position(100, 200) = #{Mouse.position.inspect}"

MOUSE_BUTTONS = {
"Left"     => Mouse::Left,
"Right"    => Mouse::Right,
"Middle"   => Mouse::Middle,
"XButton1" => Mouse::XButton1,
"XButton2" => Mouse::XButton2,
"XButton3" => Mouse::XButton3,
}

last_pos      = nil
mouse_pressed = {}

puts "\n[Close the window to exit]"
until window.should_close?
  window.update
  
  pos = Mouse.position
  if LOG_MOUSE_MOVE && pos != last_pos
    puts "Mouse moved: #{pos.inspect}"
    last_pos = pos
  end
  
  MOUSE_BUTTONS.each do |name, btn|
    is_down = Mouse.press?(btn)
    if is_down && !mouse_pressed[name]
      puts "Mouse pressed:  #{name}"
      mouse_pressed[name] = true
    elsif !is_down && mouse_pressed[name]
      puts "Mouse released: #{name}"
      mouse_pressed[name] = false
    end
  end
  
  wheel = Mouse.wheel_move
  puts "Wheel: #{wheel > 0 ? 'up' : 'down'} (#{wheel})" if wheel != 0
end

window.close_window
puts "OK"