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
// This is a deprecation shim. The aliases mirror SFML 2.6 naming
// (`Sf::Joystick` points at `LiteRGSS::Inputs::Gamepad` because SFML called
// gamepads joysticks). Future major versions should remove this file once
// downstream scripts have been ported to `LiteRGSS::Inputs::*`.

#include "Inputs.h"

#include "LiteCGSS/Events/GamepadAxis.h"

namespace {
	using Backend = cgss::backend::ActiveBackend;
	using Ops = Backend::Ops;
}

// Helper: define a constant on `mod` only if not already present. PSDK
// reads `Sf::Mouse::LEFT`; LiteRGSS3 already defines `Sf::Mouse::Left` —
// adding `LEFT` would be a clobber if anyone defined it elsewhere.
static void define_const_if_absent(VALUE mod, const char* name, VALUE value)
{
	if (rb_const_defined_at(mod, rb_intern(name))) return;
	rb_const_set(mod, rb_intern(name), value);
}

// Sf::Joystick.axis_position(id, axis) — LiteRGSS2 returned axis position
// as an integer in roughly the [-100, 100] range; LiteRGSS3's
// Gamepad.axis_position returns a float in [-1, 1]. PSDK's Input layer
// reads integer positions, so scale before exposing.
static VALUE rb_SfJoystick_axis_position(VALUE self, VALUE id, VALUE axis)
{
	(void)self;
	if (!Ops::gamepad_is_connected(NUM2UINT(id))) return INT2NUM(0);
	const float position = Ops::gamepad_axis_position(
		NUM2UINT(id),
		static_cast<cgss::GamepadAxis>(NUM2INT(axis))
	);
	return LONG2NUM(static_cast<long>(position * 100.0f));
}

// Sf::Keyboard.virtual_keyboard= — present on mobile builds of LiteRGSS2.
// Desktop is a no-op. PSDK calls this unconditionally when its in-game
// name-input scene activates.
static VALUE rb_SfKeyboard_set_virtual_keyboard(VALUE self, VALUE flag)
{
	(void)self; (void)flag;
	return Qnil;
}

// Sf::Keyboard.localize / delocalize — LiteRGSS2 mapped Scancodes to
// keyboard-layout-aware key codes. LiteRGSS3 doesn't have that distinction
// today (the canonical KeyCode enum is the layout-aware code), so these
// pass through.
static VALUE rb_SfKeyboard_localize(VALUE self, VALUE code)
{
	(void)self;
	return code;
}

void Init_SfCompatAlias()
{
	VALUE rb_mSf = rb_define_module("Sf");

	// Only alias inputs modules that have been registered. Touch is only
	// present under backends with has_input_touch=true (see Touch.cpp),
	// so skip the alias when the source module doesn't exist.
	VALUE rb_mSfKeyboard = rb_const_get(rb_mInputs, rb_intern("Keyboard"));
	VALUE rb_mSfMouse    = rb_const_get(rb_mInputs, rb_intern("Mouse"));
	VALUE rb_mSfJoystick = rb_const_get(rb_mInputs, rb_intern("Gamepad"));

	rb_const_set(rb_mSf, rb_intern("Keyboard"), rb_mSfKeyboard);
	rb_const_set(rb_mSf, rb_intern("Mouse"),    rb_mSfMouse);
	rb_const_set(rb_mSf, rb_intern("Joystick"), rb_mSfJoystick);

	if (rb_const_defined(rb_mInputs, rb_intern("Touch"))) {
		rb_const_set(rb_mSf, rb_intern("Touch"),
		             rb_const_get(rb_mInputs, rb_intern("Touch")));
	}
	rb_const_set(rb_mSf, rb_intern("Sensor"),
	             rb_const_get(rb_mInputs, rb_intern("Sensor")));

	// LiteRGSS2 compat: Scancode was a separate sub-module of Keyboard, but
	// in LiteRGSS3 the scancode constants live directly on Keyboard. Alias
	// `Scancode` back to `Keyboard` itself so `Sf::Keyboard::Scancode::C`
	// resolves to the same integer as `Sf::Keyboard::C`.
	define_const_if_absent(rb_mSfKeyboard, "Scancode", rb_mSfKeyboard);

	// LiteRGSS2 mouse button names were uppercase. LiteRGSS3 uses
	// SFML-style PascalCase (Left/Right/Middle). Provide both forms.
	define_const_if_absent(rb_mSfMouse, "LEFT",
	                       rb_const_get(rb_mSfMouse, rb_intern("Left")));
	define_const_if_absent(rb_mSfMouse, "RIGHT",
	                       rb_const_get(rb_mSfMouse, rb_intern("Right")));
	define_const_if_absent(rb_mSfMouse, "MIDDLE",
	                       rb_const_get(rb_mSfMouse, rb_intern("Middle")));

	// LiteRGSS2 / SFML axis identifiers. PSDK reads these as raw integers
	// (Sf::Joystick::Z is the trigger axis on Xbox-layout pads in SFML).
	define_const_if_absent(rb_mSfJoystick, "X",     INT2NUM(0));
	define_const_if_absent(rb_mSfJoystick, "Y",     INT2NUM(1));
	define_const_if_absent(rb_mSfJoystick, "Z",     INT2NUM(2));
	define_const_if_absent(rb_mSfJoystick, "POV_X", INT2NUM(6));
	define_const_if_absent(rb_mSfJoystick, "POV_Y", INT2NUM(7));

	// LiteRGSS2-shaped helpers PSDK calls on Sf::Joystick / Sf::Keyboard.
	rb_define_module_function(rb_mSfJoystick, "axis_position",
	                          _rbf rb_SfJoystick_axis_position, 2);
	rb_define_module_function(rb_mSfKeyboard, "virtual_keyboard=",
	                          _rbf rb_SfKeyboard_set_virtual_keyboard, 1);
	rb_define_module_function(rb_mSfKeyboard, "localize",
	                          _rbf rb_SfKeyboard_localize, 1);
	rb_define_module_function(rb_mSfKeyboard, "delocalize",
	                          _rbf rb_SfKeyboard_localize, 1);
}
