# test_gamepad_live.rb
# Interactive - click buttons, axis movement, close window to exit
require_relative '../build/lib/LiteRGSS'

puts "=== Test Gamepad Live ==="

DEAD_ZONE = 0.1
GAMEPAD_ID = 0

window = LiteRGSS::Window.new
window.open_window
puts "Window opened"

LiteRGSS::Gamepad.update

gamepad_is_connected = LiteRGSS::Gamepad.connected?(GAMEPAD_ID);
puts "Gamepad is connected      = #{gamepad_is_connected}"

# Map gamepad button names to their constants for easy iteration
GAMEPAD_BUTTONS = {
  "LeftFaceDown"     => LiteRGSS::Gamepad::BUTTON_LEFT_FACE_DOWN,
  "LeftFaceLeft"     => LiteRGSS::Gamepad::BUTTON_LEFT_FACE_LEFT,
  "LeftFaceRight"    => LiteRGSS::Gamepad::BUTTON_LEFT_FACE_RIGHT,
  "LeftFaceUp"       => LiteRGSS::Gamepad::BUTTON_LEFT_FACE_UP,
  "LeftThumb"        => LiteRGSS::Gamepad::BUTTON_LEFT_THUMB,
  "LeftTrigger1"     => LiteRGSS::Gamepad::BUTTON_LEFT_TRIGGER_1,
  "LeftTrigger2"     => LiteRGSS::Gamepad::BUTTON_LEFT_TRIGGER_2,
  "Middle"           => LiteRGSS::Gamepad::BUTTON_MIDDLE,
  "MiddleLeft"       => LiteRGSS::Gamepad::BUTTON_MIDDLE_LEFT,
  "MiddleRight"      => LiteRGSS::Gamepad::BUTTON_MIDDLE_RIGHT,
  "RightFaceDown"    => LiteRGSS::Gamepad::BUTTON_RIGHT_FACE_DOWN,
  "RightFaceLeft"    => LiteRGSS::Gamepad::BUTTON_RIGHT_FACE_LEFT,
  "RightFaceRight"   => LiteRGSS::Gamepad::BUTTON_RIGHT_FACE_RIGHT,
  "RightFaceUp"      => LiteRGSS::Gamepad::BUTTON_RIGHT_FACE_UP,
  "RightThumb"       => LiteRGSS::Gamepad::BUTTON_RIGHT_THUMB,
  "RightTrigger1"    => LiteRGSS::Gamepad::BUTTON_RIGHT_TRIGGER_1,
  "RightTrigger2"    => LiteRGSS::Gamepad::BUTTON_RIGHT_TRIGGER_2,
  "Unknown"          => LiteRGSS::Gamepad::BUTTON_UNKNOWN
}

# Map gamepad axis names to their constants for easy iteration
GAMEPAD_AXIES = {  
  "LeftX"        => LiteRGSS::Gamepad::AXIS_LEFT_X,
  "LeftY"        => LiteRGSS::Gamepad::AXIS_LEFT_Y,  
  "RightX"       => LiteRGSS::Gamepad::AXIS_RIGHT_X,
  "RightY"       => LiteRGSS::Gamepad::AXIS_RIGHT_Y
}

# Map gamepad trigger names to their constants for easy iteration
GAMEPAD_TRIGGERS = {
  "LeftTrigger"  => LiteRGSS::Gamepad::AXIS_LEFT_TRIGGER,
  "RightTrigger" => LiteRGSS::Gamepad::AXIS_RIGHT_TRIGGER,
}

if gamepad_is_connected
  puts "Gamepad name              = #{LiteRGSS::Gamepad.name(GAMEPAD_ID)}"
  puts "Axis count                = #{LiteRGSS::Gamepad.axis_count(GAMEPAD_ID)}"

  GAMEPAD_AXIES.each do |name, axis|
    puts "Axis available (#{name})    = #{LiteRGSS::Gamepad.axis_available?(GAMEPAD_ID, axis)}"
  end  
end

puts "\n[Close the window to exit]"
until window.should_close?
  window.update
  
  # Detect press and release edges for each gamepad button
  GAMEPAD_BUTTONS.each do |name, btn|
    if LiteRGSS::Gamepad.press?(GAMEPAD_ID, btn)
      puts "Button pressed:  #{name}"
    end
    if LiteRGSS::Gamepad.release?(GAMEPAD_ID, btn)
      puts "Button released: #{name}"
    end
  end

  # Detect axis mouvement of the gamepad
  GAMEPAD_AXIES.each do |name, axis|
    position = LiteRGSS::Gamepad.axis_position(0, axis)   
    puts "Axis position: #{name} #{position}" if position <= -DEAD_ZONE || position >= DEAD_ZONE
  end

  # Detect triggers of the gamepad
  GAMEPAD_TRIGGERS.each do |name, trigger|
    pressure = LiteRGSS::Gamepad.axis_position(0, trigger)   
    puts "Axis trigger: #{name} #{pressure}" if pressure > -1
  end
end

window.close_window
puts "OK"