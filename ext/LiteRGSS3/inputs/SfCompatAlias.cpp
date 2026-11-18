// Back-compat alias module for PSDK Ruby scripts that still reference the
// legacy LiteRGSS2-era `Sf::` namespace (pre-LiteRGSS3 input constants lived
// under Sf::Keyboard::Key, Sf::Mouse::Button, Sf::Joystick, etc.).
//
// Each constant here is a Ruby-level reference to the corresponding
// LiteRGSS::Inputs module. The objects are aliased, not duplicated —
// `Sf::Keyboard.equal?(LiteRGSS::Inputs::Keyboard) == true`.
//
// In addition to the aliases, this TU back-fills the LiteRGSS2-shaped
// constants and helper methods that PSDK reads at script-load time but
// LiteRGSS3 doesn't surface natively (uppercase `LEFT/RIGHT/MIDDLE` mouse
// button aliases, `Joystick::POV_X` / `POV_Y` / numeric axis ids, the
// virtual-keyboard no-op, key localize/delocalize). Adding them here
// keeps the canonical LiteRGSS::Inputs::* surface clean while still
// matching the LiteRGSS2 contract for downstream scripts.
//
// This is a deprecation shim. The aliases mirror the LiteRGSS2 Ruby naming
// (`Sf::Joystick` points at `LiteRGSS::Inputs::Gamepad` because LiteRGSS2
// called gamepads joysticks). Future major versions should remove this file once
// downstream scripts have been ported to `LiteRGSS::Inputs::*`.

#include "Inputs.h"
#include "SfJoystick.h"
#include "SfMouse.h"

#include "LiteCGSS/Events/GamepadAxis.h"
#include "LiteCGSS/Events/Scancode.h"
#include "LiteCGSS/Inputs/Keyboard.h"
#include "LiteCGSS/Inputs/Gamepad.h"
#if defined(__ANDROID__) || defined(ANDROID)
#include "LiteCGSS/Platform/Android/HostSurface.h"
#endif

// Sf::Keyboard.virtual_keyboard= — present on mobile builds of LiteRGSS2.
// PSDK calls it when its name input scene starts, the touch overlay when a
// text field is tapped. On Android the host shows its keyboard (the
// callback it armed, cgss::android::requestVirtualKeyboard); its keys come
// back as text events. Desktop is a no-op.
static VALUE rb_SfKeyboard_set_virtual_keyboard(VALUE self, VALUE flag)
{
	(void)self;
#if defined(__ANDROID__) || defined(ANDROID)
	cgss::android::requestVirtualKeyboard(RTEST(flag));
#else
	(void)flag;
#endif
	return Qnil;
}

// Sf::Keyboard.localize(scancode) -> key code with the current keyboard
// layout; delocalize(key) -> scancode. Unknown is -1, like LiteRGSS2.
static VALUE rb_SfKeyboard_localize(VALUE self, VALUE scancode)
{
	(void)self;
	return INT2NUM(static_cast<int>(cgss::keyboard::localize(static_cast<cgss::Scancode>(NUM2INT(scancode)))));
}

static VALUE rb_SfKeyboard_delocalize(VALUE self, VALUE key)
{
	(void)self;
	return INT2NUM(static_cast<int>(cgss::keyboard::delocalize(static_cast<cgss::KeyCode>(NUM2INT(key)))));
}

void Init_SfCompatAlias()
{
	VALUE rb_mSf = rb_define_module("Sf");

	// Only alias inputs modules that have been registered. Touch is only
	// present under backends with has_input_touch=true (see Touch.cpp),
	// so skip the alias when the source module doesn't exist.
	VALUE rb_mSfKeyboard = rb_const_get(rb_mInputs, rb_intern("Keyboard"));
	rb_const_set(rb_mSf, rb_intern("Keyboard"), rb_mSfKeyboard);
	// Sf::Mouse is its own module: LiteRGSS2's wheel ids.
	Init_SfMouse();
	// Sf::Joystick is its own module: LiteRGSS2's API and numbering.
	Init_SfJoystick();

	if (rb_const_defined(rb_mInputs, rb_intern("Touch"))) {
		rb_const_set(rb_mSf, rb_intern("Touch"),
		             rb_const_get(rb_mInputs, rb_intern("Touch")));
	}
	rb_const_set(rb_mSf, rb_intern("Sensor"),
	             rb_const_get(rb_mInputs, rb_intern("Sensor")));

	// Sf::Keyboard::Scancode: the physical key positions, with SFML 2.6's
	// values like LiteRGSS2 (key bindings, e.g. PSDK's input.json, store
	// them). Key callbacks receive them as their second argument.
	VALUE rb_mScancode = rb_define_module_under(rb_mSfKeyboard, "Scancode");
#define X(Name) \
	rb_define_const(rb_mScancode, #Name, INT2NUM(static_cast<int>(cgss::Scancode::Name)));
	CGSS_SCANCODE_LIST(X)
#undef X


	// LiteRGSS2-shaped helpers PSDK calls on Sf::Keyboard.
	rb_define_module_function(rb_mSfKeyboard, "virtual_keyboard=",
	                          _rbf rb_SfKeyboard_set_virtual_keyboard, 1);
	rb_define_module_function(rb_mSfKeyboard, "localize",
	                          _rbf rb_SfKeyboard_localize, 1);
	rb_define_module_function(rb_mSfKeyboard, "delocalize",
	                          _rbf rb_SfKeyboard_delocalize, 1);
}
