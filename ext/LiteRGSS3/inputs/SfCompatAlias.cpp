// Back-compat alias module for PSDK Ruby scripts that still reference the
// legacy LiteRGSS2-era `Sf::` namespace (pre-LiteRGSS3 input constants lived
// under Sf::Keyboard::Key, Sf::Mouse::Button, Sf::Joystick, etc.).
//
// Each constant here is a Ruby-level reference to the corresponding
// LiteRGSS::Inputs module. The objects are aliased, not duplicated —
// `Sf::Keyboard.equal?(LiteRGSS::Inputs::Keyboard) == true`.
//
// This is a deprecation shim. The aliases mirror SFML 2.6 naming
// (`Sf::Joystick` points at `LiteRGSS::Inputs::Gamepad` because SFML called
// gamepads joysticks). Future major versions should remove this file once
// downstream scripts have been ported to `LiteRGSS::Inputs::*`.

#include "Inputs.h"

void Init_SfCompatAlias()
{
	VALUE rb_mSf = rb_define_module("Sf");

	// Only alias inputs modules that have been registered. Touch is only
	// present under backends with has_input_touch=true (see Touch.cpp),
	// so skip the alias when the source module doesn't exist.
	rb_const_set(rb_mSf, rb_intern("Keyboard"),
	             rb_const_get(rb_mInputs, rb_intern("Keyboard")));
	rb_const_set(rb_mSf, rb_intern("Mouse"),
	             rb_const_get(rb_mInputs, rb_intern("Mouse")));
	rb_const_set(rb_mSf, rb_intern("Joystick"),
	             rb_const_get(rb_mInputs, rb_intern("Gamepad")));

	if (rb_const_defined(rb_mInputs, rb_intern("Touch"))) {
		rb_const_set(rb_mSf, rb_intern("Touch"),
		             rb_const_get(rb_mInputs, rb_intern("Touch")));
	}
}
