#include "Inputs.h"
#include "Gamepad.h"

#include "LiteCGSS/Events/GamepadAxis.h"
#include "LiteCGSS/Events/GamepadButton.h"
#include "LiteCGSS/Inputs/Gamepad.h"

VALUE rb_Gamepad_connected(VALUE self, VALUE id)
{
	(void)self;
	return cgss::gamepad::isConnected(NUM2UINT(id)) ? Qtrue : Qfalse;
}

VALUE rb_Gamepad_axis_count(VALUE self, VALUE id)
{
	(void)self;
	return UINT2NUM(cgss::gamepad::axisCount(NUM2UINT(id)));
}

VALUE rb_Gamepad_axis(VALUE self, VALUE id, VALUE axis)
{
	(void)self;
	const long vaxis = NUM2LONG(axis);
	const unsigned int axisCount = cgss::gamepad::axisCount(NUM2UINT(id));
	return (vaxis >= 0 && static_cast<unsigned long>(vaxis) < axisCount) ? Qtrue : Qfalse;
}

VALUE rb_Gamepad_press(VALUE self, VALUE id, VALUE button)
{
	(void)self;
	return cgss::gamepad::isButtonPressed(
		NUM2UINT(id),
		static_cast<cgss::GamepadButton>(NUM2INT(button))
	) ? Qtrue : Qfalse;
}

VALUE rb_Gamepad_release(VALUE self, VALUE id, VALUE button)
{
	(void)self;
	return cgss::gamepad::isButtonReleased(
		NUM2UINT(id),
		static_cast<cgss::GamepadButton>(NUM2INT(button))
	) ? Qtrue : Qfalse;
}

VALUE rb_Gamepad_axis_position(VALUE self, VALUE id, VALUE axis)
{
	(void)self;
	const float position = cgss::gamepad::axisPosition(
		NUM2UINT(id),
		static_cast<cgss::GamepadAxis>(NUM2INT(axis))
	);
	return DBL2NUM(static_cast<double>(position));
}

VALUE rb_Gamepad_update(VALUE self)
{
	cgss::gamepad::update();
	return self;
}

VALUE rb_Gamepad_name(VALUE self, VALUE id)
{
	(void)self;
	return rb_utf8_str_new_cstr(cgss::gamepad::name(NUM2UINT(id)));
}

// Vibration: no-op on backends without rumble support. Preserved in the Ruby
// API so PSDK scripts that call it keep working.
VALUE rb_Gamepad_set_vibration(VALUE self, VALUE id, VALUE leftMotor, VALUE rightMotor, VALUE duration)
{
	cgss::gamepad::setVibration(
		NUM2UINT(id),
		static_cast<float>(NUM2DBL(leftMotor)),
		static_cast<float>(NUM2DBL(rightMotor)),
		static_cast<float>(NUM2DBL(duration))
	);
	return self;
}

static void DefineGamepadConstants(VALUE rb_mGamepad)
{
	// Button constants — mapping from the legacy BUTTON_* names (which
	// existing PSDK code uses) to cgss::GamepadButton enum values.
	rb_define_const(rb_mGamepad, "BUTTON_UNKNOWN",          INT2NUM(static_cast<int>(cgss::GamepadButton::Unknown)));
	rb_define_const(rb_mGamepad, "BUTTON_LEFT_FACE_UP",     INT2NUM(static_cast<int>(cgss::GamepadButton::LeftFaceUp)));
	rb_define_const(rb_mGamepad, "BUTTON_LEFT_FACE_RIGHT",  INT2NUM(static_cast<int>(cgss::GamepadButton::LeftFaceRight)));
	rb_define_const(rb_mGamepad, "BUTTON_LEFT_FACE_DOWN",   INT2NUM(static_cast<int>(cgss::GamepadButton::LeftFaceDown)));
	rb_define_const(rb_mGamepad, "BUTTON_LEFT_FACE_LEFT",   INT2NUM(static_cast<int>(cgss::GamepadButton::LeftFaceLeft)));
	rb_define_const(rb_mGamepad, "BUTTON_RIGHT_FACE_UP",    INT2NUM(static_cast<int>(cgss::GamepadButton::RightFaceUp)));
	rb_define_const(rb_mGamepad, "BUTTON_RIGHT_FACE_RIGHT", INT2NUM(static_cast<int>(cgss::GamepadButton::RightFaceRight)));
	rb_define_const(rb_mGamepad, "BUTTON_RIGHT_FACE_DOWN",  INT2NUM(static_cast<int>(cgss::GamepadButton::RightFaceDown)));
	rb_define_const(rb_mGamepad, "BUTTON_RIGHT_FACE_LEFT",  INT2NUM(static_cast<int>(cgss::GamepadButton::RightFaceLeft)));
	rb_define_const(rb_mGamepad, "BUTTON_LEFT_TRIGGER_1",   INT2NUM(static_cast<int>(cgss::GamepadButton::LeftTrigger1)));
	rb_define_const(rb_mGamepad, "BUTTON_LEFT_TRIGGER_2",   INT2NUM(static_cast<int>(cgss::GamepadButton::LeftTrigger2)));
	rb_define_const(rb_mGamepad, "BUTTON_RIGHT_TRIGGER_1",  INT2NUM(static_cast<int>(cgss::GamepadButton::RightTrigger1)));
	rb_define_const(rb_mGamepad, "BUTTON_RIGHT_TRIGGER_2",  INT2NUM(static_cast<int>(cgss::GamepadButton::RightTrigger2)));
	rb_define_const(rb_mGamepad, "BUTTON_MIDDLE_LEFT",      INT2NUM(static_cast<int>(cgss::GamepadButton::MiddleLeft)));
	rb_define_const(rb_mGamepad, "BUTTON_MIDDLE",           INT2NUM(static_cast<int>(cgss::GamepadButton::Middle)));
	rb_define_const(rb_mGamepad, "BUTTON_MIDDLE_RIGHT",     INT2NUM(static_cast<int>(cgss::GamepadButton::MiddleRight)));
	rb_define_const(rb_mGamepad, "BUTTON_LEFT_THUMB",       INT2NUM(static_cast<int>(cgss::GamepadButton::LeftThumb)));
	rb_define_const(rb_mGamepad, "BUTTON_RIGHT_THUMB",      INT2NUM(static_cast<int>(cgss::GamepadButton::RightThumb)));

	rb_define_const(rb_mGamepad, "AXIS_LEFT_X",        INT2NUM(static_cast<int>(cgss::GamepadAxis::LeftX)));
	rb_define_const(rb_mGamepad, "AXIS_LEFT_Y",        INT2NUM(static_cast<int>(cgss::GamepadAxis::LeftY)));
	rb_define_const(rb_mGamepad, "AXIS_RIGHT_X",       INT2NUM(static_cast<int>(cgss::GamepadAxis::RightX)));
	rb_define_const(rb_mGamepad, "AXIS_RIGHT_Y",       INT2NUM(static_cast<int>(cgss::GamepadAxis::RightY)));
	rb_define_const(rb_mGamepad, "AXIS_LEFT_TRIGGER",  INT2NUM(static_cast<int>(cgss::GamepadAxis::LeftTrigger)));
	rb_define_const(rb_mGamepad, "AXIS_RIGHT_TRIGGER", INT2NUM(static_cast<int>(cgss::GamepadAxis::RightTrigger)));
}

void Init_Gamepad()
{
	VALUE rb_mGamepad = rb_define_module_under(rb_mInputs, "Gamepad");

	DefineGamepadConstants(rb_mGamepad);

	rb_define_module_function(rb_mGamepad, "connected?",      _rbf rb_Gamepad_connected,      1);
	rb_define_module_function(rb_mGamepad, "axis_count",      _rbf rb_Gamepad_axis_count,     1);
	rb_define_module_function(rb_mGamepad, "axis_available?", _rbf rb_Gamepad_axis,           2);
	rb_define_module_function(rb_mGamepad, "press?",          _rbf rb_Gamepad_press,          2);
	rb_define_module_function(rb_mGamepad, "release?",        _rbf rb_Gamepad_release,        2);
	rb_define_module_function(rb_mGamepad, "axis_position",   _rbf rb_Gamepad_axis_position,  2);
	rb_define_module_function(rb_mGamepad, "update",          _rbf rb_Gamepad_update,         0);
	rb_define_module_function(rb_mGamepad, "name",            _rbf rb_Gamepad_name,           1);
	rb_define_module_function(rb_mGamepad, "set_vibration",   _rbf rb_Gamepad_set_vibration,  4);
}
