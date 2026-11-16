// Sf::Joystick — LiteRGSS2's joystick module, in LiteRGSS2's numbering (see
// SfJoystick.h), over cgss::gamepad.

#include "SfJoystick.h"
#include "LiteRGSS.h"
#include "RubyValue.h"

#include "LiteCGSS/Inputs/Gamepad.h"

#include <iterator>

namespace legacy_joystick {

	namespace {
		using B = cgss::GamepadButton;
		using A = cgss::GamepadAxis;

		// Index = LiteRGSS2 button id.
		constexpr std::optional<B> kButtons[] = {
			B::RightFaceDown, B::RightFaceRight, B::RightFaceLeft, B::RightFaceUp,
			B::LeftTrigger1, B::RightTrigger1, B::MiddleLeft, B::MiddleRight,
			B::LeftThumb, B::RightThumb, B::Middle, std::nullopt,
			B::LeftFaceUp, B::LeftFaceDown, B::LeftFaceLeft, B::LeftFaceRight,
		};

		// Index = LiteRGSS2 axis id (X, Y, Z, R, U, V, PovX, PovY).
		constexpr A kAxes[] = {
			A::LeftX, A::LeftY, A::LeftTrigger, A::RightTrigger, A::RightX, A::RightY, A::DpadX, A::DpadY,
		};
	}

	int buttonIndex(cgss::GamepadButton b) {
		for (int i = 0; i < static_cast<int>(std::size(kButtons)); ++i) {
			if (kButtons[i] == b) return i;
		}
		return -1;
	}

	std::optional<cgss::GamepadButton> button(long index) {
		if (index < 0 || index >= static_cast<long>(std::size(kButtons))) return std::nullopt;
		return kButtons[index];
	}

	long axisId(cgss::GamepadAxis a) {
		for (long i = 0; i < static_cast<long>(std::size(kAxes)); ++i) {
			if (kAxes[i] == a) return i;
		}
		return 0;
	}

	std::optional<cgss::GamepadAxis> axis(long id) {
		if (id < 0 || id >= static_cast<long>(std::size(kAxes))) return std::nullopt;
		return kAxes[id];
	}

}

namespace {

	VALUE rb_SfJoystick_connected(VALUE self, VALUE id)
	{
		(void)self;
		return cgss::gamepad::isConnected(NUM2UINT(id)) ? Qtrue : Qfalse;
	}

	// The ids a connected pad can report (SFML gives the device's count).
	VALUE rb_SfJoystick_buttonCount(VALUE self, VALUE id)
	{
		(void)self;
		return UINT2NUM(cgss::gamepad::isConnected(NUM2UINT(id)) ? 16u : 0u);
	}

	VALUE rb_SfJoystick_axisAvailable(VALUE self, VALUE id, VALUE axis)
	{
		(void)self;
		return (cgss::gamepad::isConnected(NUM2UINT(id)) && legacy_joystick::axis(NUM2LONG(axis))) ? Qtrue : Qfalse;
	}

	VALUE rb_SfJoystick_press(VALUE self, VALUE id, VALUE button)
	{
		(void)self;
		const auto b = legacy_joystick::button(NUM2LONG(button));
		return (b && cgss::gamepad::isButtonPressed(NUM2UINT(id), *b)) ? Qtrue : Qfalse;
	}

	VALUE rb_SfJoystick_axisPosition(VALUE self, VALUE id, VALUE axis)
	{
		(void)self;
		const auto a = legacy_joystick::axis(NUM2LONG(axis));
		if (!a || !cgss::gamepad::isConnected(NUM2UINT(id))) return DBL2NUM(0.0);
		return DBL2NUM(legacy_joystick::position(cgss::gamepad::axisPosition(NUM2UINT(id), *a)));
	}

	VALUE rb_SfJoystick_update(VALUE self)
	{
		cgss::gamepad::update();
		return self;
	}

	// [name, vendor id, product id]; ids are 0 when the backend doesn't know them.
	VALUE rb_SfJoystick_identification(VALUE self, VALUE id)
	{
		(void)self;
		VALUE ary = rb_ary_new_capa(3);
		rb_ary_push(ary, rb_utf8_str_new_cstr(cgss::gamepad::name(NUM2UINT(id))));
		rb_ary_push(ary, UINT2NUM(0));
		rb_ary_push(ary, UINT2NUM(0));
		return ary;
	}

}

void Init_SfJoystick()
{
	VALUE rb_mSf = rb_define_module("Sf");
	VALUE rb_mSfJoystick = rb_define_module_under(rb_mSf, "Joystick");

	rb_define_const(rb_mSfJoystick, "COUNT", UINT2NUM(legacy_joystick::kCount));
	rb_define_const(rb_mSfJoystick, "BUTTON_COUNT", UINT2NUM(legacy_joystick::kButtonCount));
	rb_define_const(rb_mSfJoystick, "AXIS_COUNT", UINT2NUM(legacy_joystick::kAxisCount));
	const char* axes[] = { "X", "Y", "Z", "R", "U", "V", "POV_X", "POV_Y" };
	for (long i = 0; i < 8; ++i) rb_define_const(rb_mSfJoystick, axes[i], LONG2NUM(i));

	rb_define_module_function(rb_mSfJoystick, "connected?", _rbf rb_SfJoystick_connected, 1);
	rb_define_module_function(rb_mSfJoystick, "button_count", _rbf rb_SfJoystick_buttonCount, 1);
	rb_define_module_function(rb_mSfJoystick, "axis_available?", _rbf rb_SfJoystick_axisAvailable, 2);
	rb_define_module_function(rb_mSfJoystick, "press?", _rbf rb_SfJoystick_press, 2);
	rb_define_module_function(rb_mSfJoystick, "axis_position", _rbf rb_SfJoystick_axisPosition, 2);
	rb_define_module_function(rb_mSfJoystick, "update", _rbf rb_SfJoystick_update, 0);
	rb_define_module_function(rb_mSfJoystick, "identification", _rbf rb_SfJoystick_identification, 1);
}
