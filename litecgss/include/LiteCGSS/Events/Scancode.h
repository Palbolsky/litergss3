#ifndef CGSS_SCANCODE_H
#define CGSS_SCANCODE_H

// Physical key positions (scancodes), independent of the keyboard layout:
// Scancode::Q is the key left of W, whatever letter it prints. Complements
// KeyCode (the layout-aware key), see Inputs/Keyboard.h for the conversions.
//
// Numeric values are ABI: scripts store them (key bindings), so the layout
// follows SFML 2.6's sf::Keyboard::Scan exactly, as LiteRGSS2 exposed it.

namespace cgss {

	// ENTRY(Name) in value order, starting at 0. (The parameter isn't named
	// X: X is also a key name.)
	#define CGSS_SCANCODE_LIST(ENTRY) \
		ENTRY(A) ENTRY(B) ENTRY(C) ENTRY(D) ENTRY(E) ENTRY(F) ENTRY(G) ENTRY(H) ENTRY(I) ENTRY(J) ENTRY(K) ENTRY(L) ENTRY(M) ENTRY(N) ENTRY(O) ENTRY(P) \
		ENTRY(Q) ENTRY(R) ENTRY(S) ENTRY(T) ENTRY(U) ENTRY(V) ENTRY(W) ENTRY(X) ENTRY(Y) ENTRY(Z) \
		ENTRY(Num1) ENTRY(Num2) ENTRY(Num3) ENTRY(Num4) ENTRY(Num5) ENTRY(Num6) ENTRY(Num7) ENTRY(Num8) ENTRY(Num9) ENTRY(Num0) \
		ENTRY(Enter) ENTRY(Escape) ENTRY(Backspace) ENTRY(Tab) ENTRY(Space) ENTRY(Hyphen) ENTRY(Equal) ENTRY(LBracket) ENTRY(RBracket) \
		ENTRY(Backslash) ENTRY(Semicolon) ENTRY(Apostrophe) ENTRY(Grave) ENTRY(Comma) ENTRY(Period) ENTRY(Slash) \
		ENTRY(F1) ENTRY(F2) ENTRY(F3) ENTRY(F4) ENTRY(F5) ENTRY(F6) ENTRY(F7) ENTRY(F8) ENTRY(F9) ENTRY(F10) ENTRY(F11) ENTRY(F12) \
		ENTRY(F13) ENTRY(F14) ENTRY(F15) ENTRY(F16) ENTRY(F17) ENTRY(F18) ENTRY(F19) ENTRY(F20) ENTRY(F21) ENTRY(F22) ENTRY(F23) ENTRY(F24) \
		ENTRY(CapsLock) ENTRY(PrintScreen) ENTRY(ScrollLock) ENTRY(Pause) ENTRY(Insert) ENTRY(Home) ENTRY(PageUp) ENTRY(Delete) \
		ENTRY(End) ENTRY(PageDown) ENTRY(Right) ENTRY(Left) ENTRY(Down) ENTRY(Up) \
		ENTRY(NumLock) ENTRY(NumpadDivide) ENTRY(NumpadMultiply) ENTRY(NumpadMinus) ENTRY(NumpadPlus) ENTRY(NumpadEqual) \
		ENTRY(NumpadEnter) ENTRY(NumpadDecimal) ENTRY(Numpad1) ENTRY(Numpad2) ENTRY(Numpad3) ENTRY(Numpad4) ENTRY(Numpad5) \
		ENTRY(Numpad6) ENTRY(Numpad7) ENTRY(Numpad8) ENTRY(Numpad9) ENTRY(Numpad0) \
		ENTRY(NonUsBackslash) ENTRY(Application) ENTRY(Execute) ENTRY(ModeChange) ENTRY(Help) ENTRY(Menu) ENTRY(Select) \
		ENTRY(Redo) ENTRY(Undo) ENTRY(Cut) ENTRY(Copy) ENTRY(Paste) \
		ENTRY(VolumeMute) ENTRY(VolumeUp) ENTRY(VolumeDown) \
		ENTRY(MediaPlayPause) ENTRY(MediaStop) ENTRY(MediaNextTrack) ENTRY(MediaPreviousTrack) \
		ENTRY(LControl) ENTRY(LShift) ENTRY(LAlt) ENTRY(LSystem) ENTRY(RControl) ENTRY(RShift) ENTRY(RAlt) ENTRY(RSystem) \
		ENTRY(Back) ENTRY(Forward) ENTRY(Refresh) ENTRY(Stop) ENTRY(Search) ENTRY(Favorites) ENTRY(HomePage) \
		ENTRY(LaunchApplication1) ENTRY(LaunchApplication2) ENTRY(LaunchMail) ENTRY(LaunchMediaSelect)

	enum class Scancode : int {
		Unknown = -1,
	#define CGSS_SCANCODE_ENUM(Name) Name,
		CGSS_SCANCODE_LIST(CGSS_SCANCODE_ENUM)
	#undef CGSS_SCANCODE_ENUM
		ScancodeCount
	};

	static_assert(static_cast<int>(Scancode::ScancodeCount) == 146, "Scancode layout must match SFML 2.6");

}

#endif
