#ifndef CGSS_KEY_CODE_H
#define CGSS_KEY_CODE_H

// Backend-agnostic keyboard key enum. Numeric values mirror SFML 2.6
// `sf::Keyboard::Key` exactly, so `SFMLBackend::Ops` can `static_cast`
// without a lookup table. Raylib backend uses a translation table
// (see Backend/Internal/RaylibKeyTranslation.{h,cpp}) because raylib's
// KEY_* constants are sparse glyph codepoints.
//
// Numeric layout is ABI: once exposed to Ruby scripts as integer
// constants, renumbering is a breaking change. A future move to SFML 3
// (which renumbers sf::Keyboard::Key) would require shifting the
// translation burden from the raylib side to the SFML side.
//
// Values 0..100 match SFML 2.6 sf::Keyboard::Key. Values >= 101 are
// cgss extensions for keys raylib exposes that SFML 2.6's base enum
// does not (e.g. numpad Decimal/Enter/Equal, CapsLock, NumLock, etc.).
// On those keys, SFMLBackend returns `false` from keyboard_is_key_pressed
// (they can be wired up via sf::Keyboard::Scan later if needed).

namespace cgss {

	enum class KeyCode : int {
		Unknown    = -1,
		A          =  0,
		B          =  1,
		C          =  2,
		D          =  3,
		E          =  4,
		F          =  5,
		G          =  6,
		H          =  7,
		I          =  8,
		J          =  9,
		K          = 10,
		L          = 11,
		M          = 12,
		N          = 13,
		O          = 14,
		P          = 15,
		Q          = 16,
		R          = 17,
		S          = 18,
		T          = 19,
		U          = 20,
		V          = 21,
		W          = 22,
		X          = 23,
		Y          = 24,
		Z          = 25,
		Num0       = 26,
		Num1       = 27,
		Num2       = 28,
		Num3       = 29,
		Num4       = 30,
		Num5       = 31,
		Num6       = 32,
		Num7       = 33,
		Num8       = 34,
		Num9       = 35,
		Escape     = 36,
		LControl   = 37,
		LShift     = 38,
		LAlt       = 39,
		LSystem    = 40,
		RControl   = 41,
		RShift     = 42,
		RAlt       = 43,
		RSystem    = 44,
		Menu       = 45,
		LBracket   = 46,
		RBracket   = 47,
		Semicolon  = 48,
		Comma      = 49,
		Period     = 50,
		Apostrophe = 51,
		Slash      = 52,
		Backslash  = 53,
		Grave      = 54,
		Equal      = 55,
		Hyphen     = 56,
		Space      = 57,
		Enter      = 58,
		Backspace  = 59,
		Tab        = 60,
		PageUp     = 61,
		PageDown   = 62,
		End        = 63,
		Home       = 64,
		Insert     = 65,
		Delete     = 66,
		Add        = 67,
		Subtract   = 68,
		Multiply   = 69,
		Divide     = 70,
		Left       = 71,
		Right      = 72,
		Up         = 73,
		Down       = 74,
		Numpad0    = 75,
		Numpad1    = 76,
		Numpad2    = 77,
		Numpad3    = 78,
		Numpad4    = 79,
		Numpad5    = 80,
		Numpad6    = 81,
		Numpad7    = 82,
		Numpad8    = 83,
		Numpad9    = 84,
		F1         = 85,
		F2         = 86,
		F3         = 87,
		F4         = 88,
		F5         = 89,
		F6         = 90,
		F7         = 91,
		F8         = 92,
		F9         = 93,
		F10        = 94,
		F11        = 95,
		F12        = 96,
		F13        = 97,
		F14        = 98,
		F15        = 99,
		Pause      = 100,

		// cgss extensions beyond SFML 2.6's base Key enum — raylib exposes
		// these, SFML's base enum does not. SFMLBackend returns false for
		// these on keyboard_is_key_pressed.
		CapsLock       = 101,
		ScrollLock     = 102,
		NumLock        = 103,
		PrintScreen    = 104,
		NumpadDecimal  = 105,
		NumpadDivide   = 106, // distinct from KeyCode::Divide (main-row /)
		NumpadMultiply = 107, // distinct from KeyCode::Multiply
		NumpadSubtract = 108, // distinct from KeyCode::Subtract
		NumpadAdd      = 109, // distinct from KeyCode::Add
		NumpadEnter    = 110,
		NumpadEqual    = 111,
	};

	// Single source of truth for the cgss keyboard key list, used by ext/
	// to emit Ruby constants (see ext/LiteRGSS/inputs/Keyboard.cpp) and by
	// the raylib translation table to stay in sync.
	//
	// X(Name) — name as exposed on the enum and to Ruby.
	#define CGSS_KEYCODE_LIST(X) \
		X(A) X(B) X(C) X(D) X(E) X(F) X(G) X(H) X(I) X(J) X(K) X(L) X(M) \
		X(N) X(O) X(P) X(Q) X(R) X(S) X(T) X(U) X(V) X(W) X(X) X(Y) X(Z) \
		X(Num0) X(Num1) X(Num2) X(Num3) X(Num4) X(Num5) X(Num6) X(Num7) X(Num8) X(Num9) \
		X(Escape) \
		X(LControl) X(LShift) X(LAlt) X(LSystem) \
		X(RControl) X(RShift) X(RAlt) X(RSystem) \
		X(Menu) \
		X(LBracket) X(RBracket) X(Semicolon) X(Comma) X(Period) X(Apostrophe) \
		X(Slash) X(Backslash) X(Grave) X(Equal) X(Hyphen) \
		X(Space) X(Enter) X(Backspace) X(Tab) \
		X(PageUp) X(PageDown) X(End) X(Home) X(Insert) X(Delete) \
		X(Add) X(Subtract) X(Multiply) X(Divide) \
		X(Left) X(Right) X(Up) X(Down) \
		X(Numpad0) X(Numpad1) X(Numpad2) X(Numpad3) X(Numpad4) \
		X(Numpad5) X(Numpad6) X(Numpad7) X(Numpad8) X(Numpad9) \
		X(F1) X(F2) X(F3) X(F4) X(F5) X(F6) X(F7) X(F8) X(F9) \
		X(F10) X(F11) X(F12) X(F13) X(F14) X(F15) \
		X(Pause) \
		X(CapsLock) X(ScrollLock) X(NumLock) X(PrintScreen) \
		X(NumpadDecimal) X(NumpadDivide) X(NumpadMultiply) \
		X(NumpadSubtract) X(NumpadAdd) X(NumpadEnter) X(NumpadEqual)

	// Back-compat alias list — legacy key names that bindings may expose as
	// extra constants so existing scripts continue to resolve. No effect on
	// the C++ enum. Each entry: X(AliasName, CanonicalEnumMember).
	#define CGSS_KEYCODE_LEGACY_ALIAS_LIST(X) \
		X(Zero,         Num0) \
		X(One,          Num1) \
		X(Two,          Num2) \
		X(Three,        Num3) \
		X(Four,         Num4) \
		X(Five,         Num5) \
		X(Six,          Num6) \
		X(Seven,        Num7) \
		X(Eight,        Num8) \
		X(Nine,         Num9) \
		X(Minus,        Hyphen) \
		X(LSuper,       LSystem) \
		X(RSuper,       RSystem) \
		X(NumpadMinus,  NumpadSubtract) \
		X(NumpadPlus,   NumpadAdd)

}

#endif
