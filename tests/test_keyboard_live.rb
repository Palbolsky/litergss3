# test_keyboard_live.rb
# Interactive - press keys, close window to exit
require_relative '../build/lib/LiteRGSS'

Keyboard = LiteRGSS::Inputs::Keyboard

puts "=== Test Keyboard Live ==="

window = LiteRGSS::DisplayWindow.new
window.open_window
puts "Window opened"

KEYBOARD_KEYS = {
# Letters
"A" => Keyboard::A, "B" => Keyboard::B,
"C" => Keyboard::C, "D" => Keyboard::D,
"E" => Keyboard::E, "F" => Keyboard::F,
"G" => Keyboard::G, "H" => Keyboard::H,
"I" => Keyboard::I, "J" => Keyboard::J,
"K" => Keyboard::K, "L" => Keyboard::L,
"M" => Keyboard::M, "N" => Keyboard::N,
"O" => Keyboard::O, "P" => Keyboard::P,
"Q" => Keyboard::Q, "R" => Keyboard::R,
"S" => Keyboard::S, "T" => Keyboard::T,
"U" => Keyboard::U, "V" => Keyboard::V,
"W" => Keyboard::W, "X" => Keyboard::X,
"Y" => Keyboard::Y, "Z" => Keyboard::Z,
# Numbers
"Zero"  => Keyboard::Zero,  "One"   => Keyboard::One,
"Two"   => Keyboard::Two,   "Three" => Keyboard::Three,
"Four"  => Keyboard::Four,  "Five"  => Keyboard::Five,
"Six"   => Keyboard::Six,   "Seven" => Keyboard::Seven,
"Eight" => Keyboard::Eight, "Nine"  => Keyboard::Nine,
# Special
"Space"     => Keyboard::Space,
"Escape"    => Keyboard::Escape,
"Enter"     => Keyboard::Enter,
"Tab"       => Keyboard::Tab,
"Backspace" => Keyboard::Backspace,
"Insert"    => Keyboard::Insert,
"Delete"    => Keyboard::Delete,
# Arrows
"Right" => Keyboard::Right, "Left" => Keyboard::Left,
"Down"  => Keyboard::Down,  "Up"   => Keyboard::Up,
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
"F1"  => Keyboard::F1,  "F2"  => Keyboard::F2,
"F3"  => Keyboard::F3,  "F4"  => Keyboard::F4,
"F5"  => Keyboard::F5,  "F6"  => Keyboard::F6,
"F7"  => Keyboard::F7,  "F8"  => Keyboard::F8,
"F9"  => Keyboard::F9,  "F10" => Keyboard::F10,
"F11" => Keyboard::F11, "F12" => Keyboard::F12,
# Numpad
"Numpad0" => Keyboard::Numpad0, "Numpad1" => Keyboard::Numpad1,
"Numpad2" => Keyboard::Numpad2, "Numpad3" => Keyboard::Numpad3,
"Numpad4" => Keyboard::Numpad4, "Numpad5" => Keyboard::Numpad5,
"Numpad6" => Keyboard::Numpad6, "Numpad7" => Keyboard::Numpad7,
"Numpad8" => Keyboard::Numpad8, "Numpad9" => Keyboard::Numpad9,
"NumpadDecimal"  => Keyboard::NumpadDecimal,
"NumpadDivide"   => Keyboard::NumpadDivide,
"NumpadMultiply" => Keyboard::NumpadMultiply,
"NumpadMinus"    => Keyboard::NumpadMinus,
"NumpadPlus"     => Keyboard::NumpadPlus,
"NumpadEnter"    => Keyboard::NumpadEnter,
}

key_pressed = {}

puts "\n[Close the window to exit]"
until window.should_close?
  window.update
  
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
puts "OK"