#ifndef CGSS_MOUSE_BUTTON_H
#define CGSS_MOUSE_BUTTON_H

// Backend-agnostic mouse button enum. Left/Right/Middle/XButton1/XButton2
// numeric values match SFML 2.6 sf::Mouse::Button AND raylib's
// MOUSE_BUTTON_LEFT/RIGHT/MIDDLE — so `static_cast` works on both
// backends for the first three. XButton1/XButton2 map to raylib's
// MOUSE_BUTTON_SIDE/EXTRA via a small translation in RaylibBackend::Ops.
//
// VerticalWheel/HorizontalWheel are NOT real buttons — they're values
// Ruby code passes to Mouse.press? to query wheel state under raylib.
// SFML treats wheel input as events only, so mouse_is_button_pressed
// returns false for them on SFML.

namespace cgss {

	enum class MouseButton : int {
		Left            = 0,
		Right           = 1,
		Middle          = 2,
		XButton1        = 3,
		XButton2        = 4,

		// cgss extensions (not real buttons — see header comment).
		VerticalWheel   = 10,
		HorizontalWheel = 11,
	};

	#define CGSS_MOUSEBUTTON_LIST(X) \
		X(Left) X(Right) X(Middle) \
		X(XButton1) X(XButton2) \
		X(VerticalWheel) X(HorizontalWheel)

}

#endif
