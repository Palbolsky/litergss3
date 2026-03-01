require_relative './build/lib/LiteRGSS'

puts "=== LiteRGSS Test ==="

# Toggle this to mute mouse move spam in the console
LOG_MOUSE_MOVE = false

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

# Keyboard constants don't require an open window either
puts "\n[Keyboard Constants]"
puts "A        = #{LiteRGSS::Keyboard::A}"
puts "Escape   = #{LiteRGSS::Keyboard::Escape}"
puts "Enter    = #{LiteRGSS::Keyboard::Enter}"
puts "Space    = #{LiteRGSS::Keyboard::Space}"
puts "Left     = #{LiteRGSS::Keyboard::Left}"
puts "Right    = #{LiteRGSS::Keyboard::Right}"
puts "Up       = #{LiteRGSS::Keyboard::Up}"
puts "Down     = #{LiteRGSS::Keyboard::Down}"
puts "LShift   = #{LiteRGSS::Keyboard::LShift}"
puts "LControl = #{LiteRGSS::Keyboard::LControl}"

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

# Map key names to their constants for easy iteration
KEYBOARD_KEYS = {
  # Letters
  "A" => LiteRGSS::Keyboard::A,
  "B" => LiteRGSS::Keyboard::B,
  "C" => LiteRGSS::Keyboard::C,
  "D" => LiteRGSS::Keyboard::D,
  "E" => LiteRGSS::Keyboard::E,
  "F" => LiteRGSS::Keyboard::F,
  "G" => LiteRGSS::Keyboard::G,
  "H" => LiteRGSS::Keyboard::H,
  "I" => LiteRGSS::Keyboard::I,
  "J" => LiteRGSS::Keyboard::J,
  "K" => LiteRGSS::Keyboard::K,
  "L" => LiteRGSS::Keyboard::L,
  "M" => LiteRGSS::Keyboard::M,
  "N" => LiteRGSS::Keyboard::N,
  "O" => LiteRGSS::Keyboard::O,
  "P" => LiteRGSS::Keyboard::P,
  "Q" => LiteRGSS::Keyboard::Q,
  "R" => LiteRGSS::Keyboard::R,
  "S" => LiteRGSS::Keyboard::S,
  "T" => LiteRGSS::Keyboard::T,
  "U" => LiteRGSS::Keyboard::U,
  "V" => LiteRGSS::Keyboard::V,
  "W" => LiteRGSS::Keyboard::W,
  "X" => LiteRGSS::Keyboard::X,
  "Y" => LiteRGSS::Keyboard::Y,
  "Z" => LiteRGSS::Keyboard::Z,
  # Numbers
  "Zero"  => LiteRGSS::Keyboard::Zero,
  "One"   => LiteRGSS::Keyboard::One,
  "Two"   => LiteRGSS::Keyboard::Two,
  "Three" => LiteRGSS::Keyboard::Three,
  "Four"  => LiteRGSS::Keyboard::Four,
  "Five"  => LiteRGSS::Keyboard::Five,
  "Six"   => LiteRGSS::Keyboard::Six,
  "Seven" => LiteRGSS::Keyboard::Seven,
  "Eight" => LiteRGSS::Keyboard::Eight,
  "Nine"  => LiteRGSS::Keyboard::Nine,
  # Special
  "Space"     => LiteRGSS::Keyboard::Space,
  "Escape"    => LiteRGSS::Keyboard::Escape,
  "Enter"     => LiteRGSS::Keyboard::Enter,
  "Tab"       => LiteRGSS::Keyboard::Tab,
  "Backspace" => LiteRGSS::Keyboard::Backspace,
  "Insert"    => LiteRGSS::Keyboard::Insert,
  "Delete"    => LiteRGSS::Keyboard::Delete,
  # Arrows
  "Right" => LiteRGSS::Keyboard::Right,
  "Left"  => LiteRGSS::Keyboard::Left,
  "Down"  => LiteRGSS::Keyboard::Down,
  "Up"    => LiteRGSS::Keyboard::Up,
  # Navigation
  "PageUp"   => LiteRGSS::Keyboard::PageUp,
  "PageDown" => LiteRGSS::Keyboard::PageDown,
  "Home"     => LiteRGSS::Keyboard::Home,
  "End"      => LiteRGSS::Keyboard::End,
  # Modifiers
  "LShift"   => LiteRGSS::Keyboard::LShift,
  "LControl" => LiteRGSS::Keyboard::LControl,
  "LAlt"     => LiteRGSS::Keyboard::LAlt,
  "RShift"   => LiteRGSS::Keyboard::RShift,
  "RControl" => LiteRGSS::Keyboard::RControl,
  "RAlt"     => LiteRGSS::Keyboard::RAlt,
  # F keys
  "F1"  => LiteRGSS::Keyboard::F1,
  "F2"  => LiteRGSS::Keyboard::F2,
  "F3"  => LiteRGSS::Keyboard::F3,
  "F4"  => LiteRGSS::Keyboard::F4,
  "F5"  => LiteRGSS::Keyboard::F5,
  "F6"  => LiteRGSS::Keyboard::F6,
  "F7"  => LiteRGSS::Keyboard::F7,
  "F8"  => LiteRGSS::Keyboard::F8,
  "F9"  => LiteRGSS::Keyboard::F9,
  "F10" => LiteRGSS::Keyboard::F10,
  "F11" => LiteRGSS::Keyboard::F11,
  "F12" => LiteRGSS::Keyboard::F12,
  # Numpad
  "Numpad0"        => LiteRGSS::Keyboard::Numpad0,
  "Numpad1"        => LiteRGSS::Keyboard::Numpad1,
  "Numpad2"        => LiteRGSS::Keyboard::Numpad2,
  "Numpad3"        => LiteRGSS::Keyboard::Numpad3,
  "Numpad4"        => LiteRGSS::Keyboard::Numpad4,
  "Numpad5"        => LiteRGSS::Keyboard::Numpad5,
  "Numpad6"        => LiteRGSS::Keyboard::Numpad6,
  "Numpad7"        => LiteRGSS::Keyboard::Numpad7,
  "Numpad8"        => LiteRGSS::Keyboard::Numpad8,
  "Numpad9"        => LiteRGSS::Keyboard::Numpad9,
  "NumpadDecimal"  => LiteRGSS::Keyboard::NumpadDecimal,
  "NumpadDivide"   => LiteRGSS::Keyboard::NumpadDivide,
  "NumpadMultiply" => LiteRGSS::Keyboard::NumpadMultiply,
  "NumpadMinus"    => LiteRGSS::Keyboard::NumpadMinus,
  "NumpadPlus"     => LiteRGSS::Keyboard::NumpadPlus,
  "NumpadEnter"    => LiteRGSS::Keyboard::NumpadEnter,
}

last_pos      = nil
mouse_pressed = {}
key_pressed   = {}

puts "\n[Main loop - close the window to exit]"
until window.should_close?
  window.update

  # Log mouse position only when it changes (toggle with LOG_MOUSE_MOVE)
  pos = LiteRGSS::Mouse.position
  if LOG_MOUSE_MOVE && pos != last_pos
    puts "Mouse moved: #{pos.inspect}"
    last_pos = pos
  end

  # Detect press and release edges for each mouse button
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

  # Log scroll wheel direction and delta
  wheel = LiteRGSS::Mouse.wheel_move
  puts "Wheel: #{wheel > 0 ? 'up' : 'down'} (#{wheel})" if wheel != 0

  # Detect press and release edges for each keyboard key
  KEYBOARD_KEYS.each do |name, key|
    is_down = LiteRGSS::Keyboard.press?(key)
    if is_down && !key_pressed[name]
      puts "Key pressed:  #{name}"
      key_pressed[name] = true
    elsif !is_down && key_pressed[name]
      puts "Key released: #{name}"
      key_pressed[name] = false
    end
  end
end

window.close_window
puts "\n=== End of tests ==="
