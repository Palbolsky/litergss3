#ifndef SfMouse_H
#define SfMouse_H

// LiteRGSS2's wheel ids (sf::Mouse::Wheel), reported by
// DisplayWindow#on_mouse_wheel_scrolled and exposed on Sf::Mouse.
namespace sf_mouse {
	constexpr int kVerticalWheel   = 0;
	constexpr int kHorizontalWheel = 1;
}

void Init_SfMouse();

#endif
