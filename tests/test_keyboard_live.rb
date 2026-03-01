# test_keyboard_live.rb
# Interactive - press keys, close window to exit
require_relative '../build/lib/LiteRGSS'

puts "=== Test Keyboard Live ==="

window = LiteRGSS::Window.new
window.open_window
puts "Window opened"

KEYBOARD_KEYS = {
# Letters
"A" => LiteRGSS::Keyboard::A, "B" => LiteRGSS::Keyboard::B,
"C" => LiteRGSS::Keyboard::C, "D" => LiteRGSS::Keyboard::D,
"E" => LiteRGSS::Keyboard::E, "F" => LiteRGSS::Keyboard::F,
"G" => LiteRGSS::Keyboard::G, "H" => LiteRGSS::Keyboard::H,
"I" => LiteRGSS::Keyboard::I, "J" => LiteRGSS::Keyboard::J,
"K" => LiteRGSS::Keyboard::K, "L" => LiteRGSS::Keyboard::L,
"M" => LiteRGSS::Keyboard::M, "N" => LiteRGSS::Keyboard::N,
"O" => LiteRGSS::Keyboard::O, "P" => LiteRGSS::Keyboard::P,
"Q" => LiteRGSS::Keyboard::Q, "R" => LiteRGSS::Keyboard::R,
"S" => LiteRGSS::Keyboard::S, "T" => LiteRGSS::Keyboard::T,
"U" => LiteRGSS::Keyboard::U, "V" => LiteRGSS::Keyboard::V,
"W" => LiteRGSS::Keyboard::W, "X" => LiteRGSS::Keyboard::X,
"Y" => LiteRGSS::Keyboard::Y, "Z" => LiteRGSS::Keyboard::Z,
# Numbers
"Zero"  => LiteRGSS::Keyboard::Zero,  "One"   => LiteRGSS::Keyboard::One,
"Two"   => LiteRGSS::Keyboard::Two,   "Three" => LiteRGSS::Keyboard::Three,
"Four"  => LiteRGSS::Keyboard::Four,  "Five"  => LiteRGSS::Keyboard::Five,
"Six"   => LiteRGSS::Keyboard::Six,   "Seven" => LiteRGSS::Keyboard::Seven,
"Eight" => LiteRGSS::Keyboard::Eight, "Nine"  => LiteRGSS::Keyboard::Nine,
# Special
"Space"     => LiteRGSS::Keyboard::Space,
"Escape"    => LiteRGSS::Keyboard::Escape,
"Enter"     => LiteRGSS::Keyboard::Enter,
"Tab"       => LiteRGSS::Keyboard::Tab,
"Backspace" => LiteRGSS::Keyboard::Backspace,
"Insert"    => LiteRGSS::Keyboard::Insert,
"Delete"    => LiteRGSS::Keyboard::Delete,
# Arrows
"Right" => LiteRGSS::Keyboard::Right, "Left" => LiteRGSS::Keyboard::Left,
"Down"  => LiteRGSS::Keyboard::Down,  "Up"   => LiteRGSS::Keyboard::Up,
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
"F1"  => LiteRGSS::Keyboard::F1,  "F2"  => LiteRGSS::Keyboard::F2,
"F3"  => LiteRGSS::Keyboard::F3,  "F4"  => LiteRGSS::Keyboard::F4,
"F5"  => LiteRGSS::Keyboard::F5,  "F6"  => LiteRGSS::Keyboard::F6,
"F7"  => LiteRGSS::Keyboard::F7,  "F8"  => LiteRGSS::Keyboard::F8,
"F9"  => LiteRGSS::Keyboard::F9,  "F10" => LiteRGSS::Keyboard::F10,
"F11" => LiteRGSS::Keyboard::F11, "F12" => LiteRGSS::Keyboard::F12,
# Numpad
"Numpad0" => LiteRGSS::Keyboard::Numpad0, "Numpad1" => LiteRGSS::Keyboard::Numpad1,
"Numpad2" => LiteRGSS::Keyboard::Numpad2, "Numpad3" => LiteRGSS::Keyboard::Numpad3,
"Numpad4" => LiteRGSS::Keyboard::Numpad4, "Numpad5" => LiteRGSS::Keyboard::Numpad5,
"Numpad6" => LiteRGSS::Keyboard::Numpad6, "Numpad7" => LiteRGSS::Keyboard::Numpad7,
"Numpad8" => LiteRGSS::Keyboard::Numpad8, "Numpad9" => LiteRGSS::Keyboard::Numpad9,
"NumpadDecimal"  => LiteRGSS::Keyboard::NumpadDecimal,
"NumpadDivide"   => LiteRGSS::Keyboard::NumpadDivide,
"NumpadMultiply" => LiteRGSS::Keyboard::NumpadMultiply,
"NumpadMinus"    => LiteRGSS::Keyboard::NumpadMinus,
"NumpadPlus"     => LiteRGSS::Keyboard::NumpadPlus,
"NumpadEnter"    => LiteRGSS::Keyboard::NumpadEnter,
}

key_pressed = {}

puts "\n[Close the window to exit]"
until window.should_close?
  window.update
  
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
puts "OK"