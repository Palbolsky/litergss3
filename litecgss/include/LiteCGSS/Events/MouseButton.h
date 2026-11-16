#ifndef CGSS_MOUSE_BUTTON_H
#define CGSS_MOUSE_BUTTON_H

// Backend-agnostic mouse button enum. Each backend maps it to its native
// buttons in its Ops. Numeric values are ABI (exposed to scripts).
//
// VerticalWheel/HorizontalWheel are NOT real buttons — they're values
// scripts pass to Mouse.press? to query wheel state. Backends that only
// report the wheel through events return false for them.

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
