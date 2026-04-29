# test_gamepad_live.rb
# Interactive - click buttons, axis movement, close window to exit
require_relative '../build/lib/LiteRGSS'

Gamepad = LiteRGSS::Inputs::Gamepad

puts "=== Test Gamepad Live ==="

DEAD_ZONE = 0.1
GAMEPAD_ID = 0

window = LiteRGSS::DisplayWindow.new
window.open_window
puts "Window opened"

Gamepad.update

gamepad_is_connected = Gamepad.connected?(GAMEPAD_ID);
puts "Gamepad is connected      = #{gamepad_is_connected}"

# Map gamepad button names to their constants for easy iteration
GAMEPAD_BUTTONS = {
  "LeftFaceDown"     => Gamepad::BUTTON_LEFT_FACE_DOWN,
  "LeftFaceLeft"     => Gamepad::BUTTON_LEFT_FACE_LEFT,
  "LeftFaceRight"    => Gamepad::BUTTON_LEFT_FACE_RIGHT,
  "LeftFaceUp"       => Gamepad::BUTTON_LEFT_FACE_UP,
  "LeftThumb"        => Gamepad::BUTTON_LEFT_THUMB,
  "LeftTrigger1"     => Gamepad::BUTTON_LEFT_TRIGGER_1,
  "LeftTrigger2"     => Gamepad::BUTTON_LEFT_TRIGGER_2,
  "Middle"           => Gamepad::BUTTON_MIDDLE,
  "MiddleLeft"       => Gamepad::BUTTON_MIDDLE_LEFT,
  "MiddleRight"      => Gamepad::BUTTON_MIDDLE_RIGHT,
  "RightFaceDown"    => Gamepad::BUTTON_RIGHT_FACE_DOWN,
  "RightFaceLeft"    => Gamepad::BUTTON_RIGHT_FACE_LEFT,
  "RightFaceRight"   => Gamepad::BUTTON_RIGHT_FACE_RIGHT,
  "RightFaceUp"      => Gamepad::BUTTON_RIGHT_FACE_UP,
  "RightThumb"       => Gamepad::BUTTON_RIGHT_THUMB,
  "RightTrigger1"    => Gamepad::BUTTON_RIGHT_TRIGGER_1,
  "RightTrigger2"    => Gamepad::BUTTON_RIGHT_TRIGGER_2,
  "Unknown"          => Gamepad::BUTTON_UNKNOWN
}

# Map gamepad axis names to their constants for easy iteration
GAMEPAD_AXIES = {  
  "LeftX"        => Gamepad::AXIS_LEFT_X,
  "LeftY"        => Gamepad::AXIS_LEFT_Y,  
  "RightX"       => Gamepad::AXIS_RIGHT_X,
  "RightY"       => Gamepad::AXIS_RIGHT_Y
}

# Map gamepad trigger names to their constants for easy iteration
GAMEPAD_TRIGGERS = {
  "LeftTrigger"  => Gamepad::AXIS_LEFT_TRIGGER,
  "RightTrigger" => Gamepad::AXIS_RIGHT_TRIGGER,
}

if gamepad_is_connected
  puts "Gamepad name              = #{Gamepad.name(GAMEPAD_ID)}"
  puts "Axis count                = #{Gamepad.axis_count(GAMEPAD_ID)}"

  GAMEPAD_AXIES.each do |name, axis|
    puts "Axis available (#{name})    = #{Gamepad.axis_available?(GAMEPAD_ID, axis)}"
  end  
end

puts "\n[Close the window to exit]"
until window.should_close?
  window.update
  
  # Detect press and release edges for each gamepad button
  GAMEPAD_BUTTONS.each do |name, btn|
    if Gamepad.press?(GAMEPAD_ID, btn)
      puts "Button pressed:  #{name}"
    end
    if Gamepad.release?(GAMEPAD_ID, btn)
      puts "Button released: #{name}"
    end
  end

  # Detect axis mouvement of the gamepad
  GAMEPAD_AXIES.each do |name, axis|
    position = Gamepad.axis_position(0, axis)   
    puts "Axis position: #{name} #{position}" if position <= -DEAD_ZONE || position >= DEAD_ZONE
  end

  # Detect triggers of the gamepad
  GAMEPAD_TRIGGERS.each do |name, trigger|
    pressure = Gamepad.axis_position(0, trigger)   
    puts "Axis trigger: #{name} #{pressure}" if pressure > -1
  end
end

window.close_window
puts "OK"