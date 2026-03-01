require_relative './build/lib/LiteRGSS'

puts "=== LiteRGSS Test ==="

# Check that base modules are accessible
puts "\n[Modules]"
p LiteRGSS::Error
p LiteRGSS::Config

# Mouse button constants don't require an open window
puts "\n[Mouse Constants]"
puts "Left        = #{LiteRGSS::Mouse::Left}"
puts "LEFT        = #{LiteRGSS::Mouse::LEFT}"
puts "Right       = #{LiteRGSS::Mouse::Right}"
puts "RIGHT       = #{LiteRGSS::Mouse::RIGHT}"
puts "Middle      = #{LiteRGSS::Mouse::Middle}"
puts "XButton1    = #{LiteRGSS::Mouse::XButton1}"
puts "XButton2    = #{LiteRGSS::Mouse::XButton2}"
puts "VertWheel   = #{LiteRGSS::Mouse::VerticalWheel}"
puts "HorizWheel  = #{LiteRGSS::Mouse::HorizontalWheel}"

# Open the window before any input polling
puts "\n[Window]"
window = LiteRGSS::Window.new
window.open_window
puts "Window opened"

# Snapshot of mouse state right after window init
puts "\n[Mouse State]"
puts "Position    = #{LiteRGSS::Mouse.position.inspect}"
puts "Press Left  = #{LiteRGSS::Mouse.press?(LiteRGSS::Mouse::Left)}"
puts "Press Right = #{LiteRGSS::Mouse.press?(LiteRGSS::Mouse::Right)}"

# Warp the cursor to a fixed position and verify
puts "\n[Mouse Set Position]"
LiteRGSS::Mouse.set_position(100, 200)
puts "After set_position(100, 200) = #{LiteRGSS::Mouse.position.inspect}"

# Map button names to their constants for easy iteration
MOUSE_BUTTONS = {
"Left"     => LiteRGSS::Mouse::Left,
"Right"    => LiteRGSS::Mouse::Right,
"Middle"   => LiteRGSS::Mouse::Middle,
"XButton1" => LiteRGSS::Mouse::XButton1,
"XButton2" => LiteRGSS::Mouse::XButton2,
}

last_pos = nil
pressed  = {}

puts "\n[Main loop - close the window to exit]"
until window.should_close?
  window.update
  
  # Log mouse position only when it changes
  pos = LiteRGSS::Mouse.position
  if pos != last_pos
    puts "Mouse moved: #{pos.inspect}"
    last_pos = pos
  end
  
  # Detect press and release edges for each button
  MOUSE_BUTTONS.each do |name, btn|
    is_down = LiteRGSS::Mouse.press?(btn)
    if is_down && !pressed[name]
      puts "Button pressed:  #{name}"
      pressed[name] = true
    elsif !is_down && pressed[name]
      puts "Button released: #{name}"
      pressed[name] = false
    end
  end
  
  # Log scroll wheel direction and delta
  wheel = LiteRGSS::Mouse.wheel_move
  if wheel != 0
    puts "Wheel: #{wheel > 0 ? 'up' : 'down'} (#{wheel})"
  end
end

window.close_window
puts "\n=== End of tests ==="