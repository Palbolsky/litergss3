# test_joystick_legacy.rb
require 'minitest/autorun'
require_relative 'litergss'

# Sf::Joystick keeps LiteRGSS2's API and numbering (SFML's raw ids), which
# PSDK's key bindings and AXIS_MAPPING are written against.
class TestJoystickLegacy < Minitest::Test
  J = Sf::Joystick

  def test_own_module
    refute_same LiteRGSS::Inputs::Gamepad, J
  end

  def test_constants_match_litergss2
    { COUNT: 8, BUTTON_COUNT: 32, AXIS_COUNT: 8, X: 0, Y: 1, Z: 2, R: 3, U: 4, V: 5, POV_X: 6, POV_Y: 7 }.each do |name, value|
      assert_equal value, J.const_get(name), "Joystick::#{name}"
    end
  end

  def test_disconnected_pad_answers
    refute J.connected?(7)
    assert_equal 0, J.button_count(7)
    refute J.press?(7, 0)
    refute J.axis_available?(7, J::X)
    assert_kind_of Float, J.axis_position(7, J::Z)
    assert_equal 3, J.identification(7).size
  end
end
