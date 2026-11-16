# test_keyboard_scancode.rb
require 'minitest/autorun'
require_relative 'litergss'

# Sf::Keyboard::Scancode carries SFML 2.6's physical key values, as in
# LiteRGSS2: PSDK's key bindings (and saved input.json) store them.
class TestKeyboardScancode < Minitest::Test
  S = Sf::Keyboard::Scancode
  K = Sf::Keyboard

  def test_scancode_is_its_own_module
    refute_same K, S
  end

  def test_scancode_values_match_litergss2
    expected = { A: 0, Z: 25, Num1: 26, Num0: 35, Enter: 36, Escape: 37, Backspace: 38, Tab: 39, Space: 40,
                 Apostrophe: 47, Grave: 48, F1: 52, CapsLock: 76, Right: 86, Up: 89, NumpadEnter: 96,
                 Numpad0: 107, LControl: 127, RSystem: 134, LaunchMediaSelect: 145 }
    expected.each { |name, value| assert_equal value, S.const_get(name), "Scancode::#{name}" }
    assert_equal 146, S.constants.size
  end

  def test_keyboard_keeps_sfml_key_values
    assert_equal 0, K::A
    assert_equal 58, K::Enter
    assert_equal 51, K::Quote
    assert_equal 54, K::Tilde
  end

  def test_localize_and_delocalize
    assert_equal K::A, K.localize(S::A)
    assert_equal K::Enter, K.localize(S::Enter)
    assert_equal S::Enter, K.delocalize(K::Enter)
    assert_equal S::Space, K.delocalize(K::Space)
    assert_equal(-1, K.localize(-1))
  end
end
