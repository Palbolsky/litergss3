require_relative './build/lib/LiteRGSS'

puts "=== LiteRGSS Test ==="

# Toggle this to mute mouse move spam in the console
LOG_MOUSE_MOVE = false

Mouse    = LiteRGSS::Inputs::Mouse
Keyboard = LiteRGSS::Inputs::Keyboard

# Check that base modules are accessible
puts "\n[Modules]"
p LiteRGSS::Error
p LiteRGSS::Config
p Mouse
p Keyboard

# Mouse button constants don't require an open window.
# Constants are PascalCase (sourced from cgss::MouseButton).
puts "\n[Mouse Constants]"
puts "Left        = #{Mouse::Left}"
puts "Right       = #{Mouse::Right}"
puts "Middle      = #{Mouse::Middle}"
puts "XButton1    = #{Mouse::XButton1}"
puts "XButton2    = #{Mouse::XButton2}"
puts "VertWheel   = #{Mouse::VerticalWheel}"
puts "HorizWheel  = #{Mouse::HorizontalWheel}"

# Keyboard constants don't require an open window either.
# Canonical names come from cgss::KeyCode (SFML 2.6 layout);
# Zero..Nine are raylib-style aliases for Num0..Num9.
puts "\n[Keyboard Constants]"
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

# Open the window before any input polling
puts "\n[Window]"
window = LiteRGSS::DisplayWindow.new
window.open_window
puts "Window opened"

# Snapshot of mouse state right after window init
puts "\n[Mouse State]"
puts "Position    = #{Mouse.position.inspect}"
puts "Press Left  = #{Mouse.press?(Mouse::Left)}"
puts "Press Right = #{Mouse.press?(Mouse::Right)}"

# Warp the cursor to a fixed position and verify
puts "\n[Mouse Set Position]"
Mouse.set_position(100, 200)
puts "After set_position(100, 200) = #{Mouse.position.inspect}"

# Map button names to their constants for easy iteration
MOUSE_BUTTONS = {
  "Left"     => Mouse::Left,
  "Right"    => Mouse::Right,
  "Middle"   => Mouse::Middle,
  "XButton1" => Mouse::XButton1,
  "XButton2" => Mouse::XButton2,
}

# Map key names to their constants for easy iteration
KEYBOARD_KEYS = {
  # Letters
  "A" => Keyboard::A, "B" => Keyboard::B, "C" => Keyboard::C, "D" => Keyboard::D,
  "E" => Keyboard::E, "F" => Keyboard::F, "G" => Keyboard::G, "H" => Keyboard::H,
  "I" => Keyboard::I, "J" => Keyboard::J, "K" => Keyboard::K, "L" => Keyboard::L,
  "M" => Keyboard::M, "N" => Keyboard::N, "O" => Keyboard::O, "P" => Keyboard::P,
  "Q" => Keyboard::Q, "R" => Keyboard::R, "S" => Keyboard::S, "T" => Keyboard::T,
  "U" => Keyboard::U, "V" => Keyboard::V, "W" => Keyboard::W, "X" => Keyboard::X,
  "Y" => Keyboard::Y, "Z" => Keyboard::Z,
  # Numbers (Zero..Nine are raylib-style aliases for Num0..Num9)
  "Zero"  => Keyboard::Zero,  "One"   => Keyboard::One,   "Two"   => Keyboard::Two,
  "Three" => Keyboard::Three, "Four"  => Keyboard::Four,  "Five"  => Keyboard::Five,
  "Six"   => Keyboard::Six,   "Seven" => Keyboard::Seven, "Eight" => Keyboard::Eight,
  "Nine"  => Keyboard::Nine,
  # Special
  "Space"     => Keyboard::Space,
  "Escape"    => Keyboard::Escape,
  "Enter"     => Keyboard::Enter,
  "Tab"       => Keyboard::Tab,
  "Backspace" => Keyboard::Backspace,
  "Insert"    => Keyboard::Insert,
  "Delete"    => Keyboard::Delete,
  # Arrows
  "Right" => Keyboard::Right,
  "Left"  => Keyboard::Left,
  "Down"  => Keyboard::Down,
  "Up"    => Keyboard::Up,
  # Navigation
  "PageUp"   => Keyboard::PageUp,
  "PageDown" => Keyboard::PageDown,
  "Home"     => Keyboard::Home,
  "End"      => Keyboard::End,
  # Modifiers
  "LShift"   => Keyboard::LShift,
  "LControl" => Keyboard::LControl,
  "LAlt"     => Keyboard::LAlt,
  "RShift"   => Keyboard::RShift,
  "RControl" => Keyboard::RControl,
  "RAlt"     => Keyboard::RAlt,
  # F keys
  "F1"  => Keyboard::F1,  "F2"  => Keyboard::F2,  "F3"  => Keyboard::F3,
  "F4"  => Keyboard::F4,  "F5"  => Keyboard::F5,  "F6"  => Keyboard::F6,
  "F7"  => Keyboard::F7,  "F8"  => Keyboard::F8,  "F9"  => Keyboard::F9,
  "F10" => Keyboard::F10, "F11" => Keyboard::F11, "F12" => Keyboard::F12,
  # Numpad (NumpadMinus/NumpadPlus are raylib-style aliases)
  "Numpad0"        => Keyboard::Numpad0,
  "Numpad1"        => Keyboard::Numpad1,
  "Numpad2"        => Keyboard::Numpad2,
  "Numpad3"        => Keyboard::Numpad3,
  "Numpad4"        => Keyboard::Numpad4,
  "Numpad5"        => Keyboard::Numpad5,
  "Numpad6"        => Keyboard::Numpad6,
  "Numpad7"        => Keyboard::Numpad7,
  "Numpad8"        => Keyboard::Numpad8,
  "Numpad9"        => Keyboard::Numpad9,
  "NumpadDecimal"  => Keyboard::NumpadDecimal,
  "NumpadDivide"   => Keyboard::NumpadDivide,
  "NumpadMultiply" => Keyboard::NumpadMultiply,
  "NumpadMinus"    => Keyboard::NumpadMinus,
  "NumpadPlus"     => Keyboard::NumpadPlus,
  "NumpadEnter"    => Keyboard::NumpadEnter,
}

last_pos      = nil
mouse_pressed = {}
key_pressed   = {}

puts "\n[Main loop - close the window to exit]"
until window.should_close?
  window.update

  # Log mouse position only when it changes (toggle with LOG_MOUSE_MOVE)
  pos = Mouse.position
  if LOG_MOUSE_MOVE && pos != last_pos
    puts "Mouse moved: #{pos.inspect}"
    last_pos = pos
  end

  # Detect press and release edges for each mouse button
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

  # Log scroll wheel direction and delta
  wheel = Mouse.wheel_move
  puts "Wheel: #{wheel > 0 ? 'up' : 'down'} (#{wheel})" if wheel != 0

  # Detect press and release edges for each keyboard key
  KEYBOARD_KEYS.each do |name, key|
    is_down = Keyboard.press?(key)
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
