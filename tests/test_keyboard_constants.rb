# test_keyboard_constants.rb
require 'minitest/autorun'
require_relative '../build/lib/LiteRGSS'

class TestKeyboardConstants < Minitest::Test
  Keyboard = LiteRGSS::Inputs::Keyboard

  EXPECTED_CONSTANTS = %w[
    Apostrophe
    Comma
    Minus
    Period
    Slash
    Zero
    One
    Two
    Three
    Four
    Five
    Six
    Seven
    Eight
    Nine
    Semicolon
    Equal
    A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
    LBracket
    Backslash
    RBracket
    Grave
    Space
    Escape
    Enter
    Tab
    Backspace
    Insert
    Delete
    Right
    Left
    Down
    Up
    PageUp
    PageDown
    Home
    End
    CapsLock
    ScrollLock
    NumLock
    PrintScreen
    Pause
    F1 F2 F3 F4 F5 F6 F7 F8 F9 F10 F11 F12
    LShift
    LControl
    LAlt
    LSuper
    RShift
    RControl
    RAlt
    RSuper
    Menu
    Numpad0 Numpad1 Numpad2 Numpad3 Numpad4
    Numpad5 Numpad6 Numpad7 Numpad8 Numpad9
    NumpadDecimal
    NumpadDivide
    NumpadMultiply
    NumpadMinus
    NumpadPlus
    NumpadEnter
    NumpadEqual
  ]

  EXPECTED_CONSTANTS.each do |const|
    define_method("test_#{const.downcase}_exists") do
      assert Keyboard.const_defined?(const), "#{const} is not defined"
      assert_kind_of Integer, Keyboard.const_get(const), "#{const} should be an Integer"
    end
  end
end