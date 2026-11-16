#include "LiteCGSS/Backend/Internal/RaylibKeyTranslation.h"

#include <raylib.h>

namespace cgss::backend::raylib_detail {

	int toRaylibKey(cgss::KeyCode code) noexcept {
		using K = cgss::KeyCode;
		switch (code) {
			// Letters
			case K::A: return KEY_A; case K::B: return KEY_B; case K::C: return KEY_C;
			case K::D: return KEY_D; case K::E: return KEY_E; case K::F: return KEY_F;
			case K::G: return KEY_G; case K::H: return KEY_H; case K::I: return KEY_I;
			case K::J: return KEY_J; case K::K: return KEY_K; case K::L: return KEY_L;
			case K::M: return KEY_M; case K::N: return KEY_N; case K::O: return KEY_O;
			case K::P: return KEY_P; case K::Q: return KEY_Q; case K::R: return KEY_R;
			case K::S: return KEY_S; case K::T: return KEY_T; case K::U: return KEY_U;
			case K::V: return KEY_V; case K::W: return KEY_W; case K::X: return KEY_X;
			case K::Y: return KEY_Y; case K::Z: return KEY_Z;
			// Digits (main row)
			case K::Num0: return KEY_ZERO;  case K::Num1: return KEY_ONE;
			case K::Num2: return KEY_TWO;   case K::Num3: return KEY_THREE;
			case K::Num4: return KEY_FOUR;  case K::Num5: return KEY_FIVE;
			case K::Num6: return KEY_SIX;   case K::Num7: return KEY_SEVEN;
			case K::Num8: return KEY_EIGHT; case K::Num9: return KEY_NINE;
			// Modifiers / system
			case K::Escape:    return KEY_ESCAPE;
			case K::LControl:  return KEY_LEFT_CONTROL;
			case K::LShift:    return KEY_LEFT_SHIFT;
			case K::LAlt:      return KEY_LEFT_ALT;
			case K::LSystem:   return KEY_LEFT_SUPER;
			case K::RControl:  return KEY_RIGHT_CONTROL;
			case K::RShift:    return KEY_RIGHT_SHIFT;
			case K::RAlt:      return KEY_RIGHT_ALT;
			case K::RSystem:   return KEY_RIGHT_SUPER;
			case K::Menu:      return KEY_KB_MENU;
			// Punctuation
			case K::LBracket:   return KEY_LEFT_BRACKET;
			case K::RBracket:   return KEY_RIGHT_BRACKET;
			case K::Semicolon:  return KEY_SEMICOLON;
			case K::Comma:      return KEY_COMMA;
			case K::Period:     return KEY_PERIOD;
			case K::Apostrophe: return KEY_APOSTROPHE;
			case K::Slash:      return KEY_SLASH;
			case K::Backslash:  return KEY_BACKSLASH;
			case K::Grave:      return KEY_GRAVE;
			case K::Equal:      return KEY_EQUAL;
			case K::Hyphen:     return KEY_MINUS;
			// Whitespace / edit
			case K::Space:     return KEY_SPACE;
			case K::Enter:     return KEY_ENTER;
			case K::Backspace: return KEY_BACKSPACE;
			case K::Tab:       return KEY_TAB;
			// Navigation
			case K::PageUp:   return KEY_PAGE_UP;
			case K::PageDown: return KEY_PAGE_DOWN;
			case K::End:      return KEY_END;
			case K::Home:     return KEY_HOME;
			case K::Insert:   return KEY_INSERT;
			case K::Delete:   return KEY_DELETE;
			// Main-row arithmetic — raylib has no distinct main-row keys for
			// these, only the keypad variants. Route cgss::Add/Subtract/etc
			// (main row) to their keypad equivalents as best effort.
			case K::Add:      return KEY_KP_ADD;
			case K::Subtract: return KEY_KP_SUBTRACT;
			case K::Multiply: return KEY_KP_MULTIPLY;
			case K::Divide:   return KEY_KP_DIVIDE;
			// Arrow keys
			case K::Left:  return KEY_LEFT;
			case K::Right: return KEY_RIGHT;
			case K::Up:    return KEY_UP;
			case K::Down:  return KEY_DOWN;
			// Numpad digits
			case K::Numpad0: return KEY_KP_0; case K::Numpad1: return KEY_KP_1;
			case K::Numpad2: return KEY_KP_2; case K::Numpad3: return KEY_KP_3;
			case K::Numpad4: return KEY_KP_4; case K::Numpad5: return KEY_KP_5;
			case K::Numpad6: return KEY_KP_6; case K::Numpad7: return KEY_KP_7;
			case K::Numpad8: return KEY_KP_8; case K::Numpad9: return KEY_KP_9;
			// Function keys — raylib only ships F1..F12. F13/F14/F15 unmapped.
			case K::F1:  return KEY_F1;  case K::F2:  return KEY_F2;
			case K::F3:  return KEY_F3;  case K::F4:  return KEY_F4;
			case K::F5:  return KEY_F5;  case K::F6:  return KEY_F6;
			case K::F7:  return KEY_F7;  case K::F8:  return KEY_F8;
			case K::F9:  return KEY_F9;  case K::F10: return KEY_F10;
			case K::F11: return KEY_F11; case K::F12: return KEY_F12;
			case K::F13: case K::F14: case K::F15: return KEY_NULL;
			case K::Pause: return KEY_PAUSE;
			// cgss extensions (raylib-native, not in SFML 2.6 base enum)
			case K::CapsLock:       return KEY_CAPS_LOCK;
			case K::ScrollLock:     return KEY_SCROLL_LOCK;
			case K::NumLock:        return KEY_NUM_LOCK;
			case K::PrintScreen:    return KEY_PRINT_SCREEN;
			case K::NumpadDecimal:  return KEY_KP_DECIMAL;
			case K::NumpadDivide:   return KEY_KP_DIVIDE;
			case K::NumpadMultiply: return KEY_KP_MULTIPLY;
			case K::NumpadSubtract: return KEY_KP_SUBTRACT;
			case K::NumpadAdd:      return KEY_KP_ADD;
			case K::NumpadEnter:    return KEY_KP_ENTER;
			case K::NumpadEqual:    return KEY_KP_EQUAL;
			case K::Unknown:        return KEY_NULL;
		}
		return KEY_NULL;
	}

	cgss::KeyCode fromRaylibKey(int raylibKey) noexcept {
		using K = cgss::KeyCode;
		switch (raylibKey) {
			case KEY_A: return K::A; case KEY_B: return K::B; case KEY_C: return K::C;
			case KEY_D: return K::D; case KEY_E: return K::E; case KEY_F: return K::F;
			case KEY_G: return K::G; case KEY_H: return K::H; case KEY_I: return K::I;
			case KEY_J: return K::J; case KEY_K: return K::K; case KEY_L: return K::L;
			case KEY_M: return K::M; case KEY_N: return K::N; case KEY_O: return K::O;
			case KEY_P: return K::P; case KEY_Q: return K::Q; case KEY_R: return K::R;
			case KEY_S: return K::S; case KEY_T: return K::T; case KEY_U: return K::U;
			case KEY_V: return K::V; case KEY_W: return K::W; case KEY_X: return K::X;
			case KEY_Y: return K::Y; case KEY_Z: return K::Z;
			case KEY_ZERO: return K::Num0;  case KEY_ONE:  return K::Num1;
			case KEY_TWO:  return K::Num2;  case KEY_THREE: return K::Num3;
			case KEY_FOUR: return K::Num4;  case KEY_FIVE: return K::Num5;
			case KEY_SIX:  return K::Num6;  case KEY_SEVEN: return K::Num7;
			case KEY_EIGHT: return K::Num8; case KEY_NINE: return K::Num9;
			case KEY_ESCAPE:        return K::Escape;
			case KEY_LEFT_CONTROL:  return K::LControl;
			case KEY_LEFT_SHIFT:    return K::LShift;
			case KEY_LEFT_ALT:      return K::LAlt;
			case KEY_LEFT_SUPER:    return K::LSystem;
			case KEY_RIGHT_CONTROL: return K::RControl;
			case KEY_RIGHT_SHIFT:   return K::RShift;
			case KEY_RIGHT_ALT:     return K::RAlt;
			case KEY_RIGHT_SUPER:   return K::RSystem;
			case KEY_KB_MENU:       return K::Menu;
			case KEY_LEFT_BRACKET:  return K::LBracket;
			case KEY_RIGHT_BRACKET: return K::RBracket;
			case KEY_SEMICOLON:     return K::Semicolon;
			case KEY_COMMA:         return K::Comma;
			case KEY_PERIOD:        return K::Period;
			case KEY_APOSTROPHE:    return K::Apostrophe;
			case KEY_SLASH:         return K::Slash;
			case KEY_BACKSLASH:     return K::Backslash;
			case KEY_GRAVE:         return K::Grave;
			case KEY_EQUAL:         return K::Equal;
			case KEY_MINUS:         return K::Hyphen;
			case KEY_SPACE:         return K::Space;
			case KEY_ENTER:         return K::Enter;
			case KEY_BACKSPACE:     return K::Backspace;
			case KEY_TAB:           return K::Tab;
			case KEY_PAGE_UP:       return K::PageUp;
			case KEY_PAGE_DOWN:     return K::PageDown;
			case KEY_END:           return K::End;
			case KEY_HOME:          return K::Home;
			case KEY_INSERT:        return K::Insert;
			case KEY_DELETE:        return K::Delete;
			case KEY_LEFT:          return K::Left;
			case KEY_RIGHT:         return K::Right;
			case KEY_UP:            return K::Up;
			case KEY_DOWN:          return K::Down;
			case KEY_KP_0: return K::Numpad0; case KEY_KP_1: return K::Numpad1;
			case KEY_KP_2: return K::Numpad2; case KEY_KP_3: return K::Numpad3;
			case KEY_KP_4: return K::Numpad4; case KEY_KP_5: return K::Numpad5;
			case KEY_KP_6: return K::Numpad6; case KEY_KP_7: return K::Numpad7;
			case KEY_KP_8: return K::Numpad8; case KEY_KP_9: return K::Numpad9;
			case KEY_F1:  return K::F1;  case KEY_F2:  return K::F2;
			case KEY_F3:  return K::F3;  case KEY_F4:  return K::F4;
			case KEY_F5:  return K::F5;  case KEY_F6:  return K::F6;
			case KEY_F7:  return K::F7;  case KEY_F8:  return K::F8;
			case KEY_F9:  return K::F9;  case KEY_F10: return K::F10;
			case KEY_F11: return K::F11; case KEY_F12: return K::F12;
			case KEY_PAUSE:         return K::Pause;
			case KEY_CAPS_LOCK:     return K::CapsLock;
			case KEY_SCROLL_LOCK:   return K::ScrollLock;
			case KEY_NUM_LOCK:      return K::NumLock;
			case KEY_PRINT_SCREEN:  return K::PrintScreen;
			case KEY_KP_DECIMAL:    return K::NumpadDecimal;
			case KEY_KP_DIVIDE:     return K::NumpadDivide;
			case KEY_KP_MULTIPLY:   return K::NumpadMultiply;
			case KEY_KP_SUBTRACT:   return K::NumpadSubtract;
			case KEY_KP_ADD:        return K::NumpadAdd;
			case KEY_KP_ENTER:      return K::NumpadEnter;
			case KEY_KP_EQUAL:      return K::NumpadEqual;
			default: return K::Unknown;
		}
	}

}
