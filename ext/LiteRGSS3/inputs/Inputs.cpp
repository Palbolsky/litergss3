#include "Inputs.h"
#include "LiteRGSS.h"
#include "Mouse.h"
#include "Keyboard.h"
#include "Touch.h"
#include "Gamepad.h"
#include "Sensor.h"

VALUE rb_mInputs = Qnil;

void Init_SfCompatAlias();

void Init_Inputs()
{
	rb_mInputs = rb_define_module_under(rb_mLiteRGSS, "Inputs");

	Init_Mouse();
	Init_Keyboard();
	Init_Touch();
	Init_Gamepad();
	Init_Sensor();

	// Legacy Sf::* aliases for PSDK scripts predating LiteRGSS3. Deprecation
	// documented in SfCompatAlias.cpp. Runs last so all source modules exist.
	Init_SfCompatAlias();
}