#include "Inputs.h"
#include "Gamepad.h"

VALUE rb_Gamepad_connected(VALUE self, VALUE id)
{
    return raylib::IsGamepadAvailable(NUM2UINT(id)) ? Qtrue : Qfalse;
}

VALUE rb_Gamepad_axis_count(VALUE self, VALUE id)
{
    return INT2NUM(raylib::GetGamepadAxisCount(NUM2UINT(id)));
}

VALUE rb_Gamepad_axis(VALUE self, VALUE id, VALUE axis)
{
    auto vaxis = NUM2LONG(axis);
    int gamepadId = NUM2UINT(id);
    int axisCount = raylib::GetGamepadAxisCount(gamepadId);

    return (vaxis >= 0 && vaxis < axisCount) ? Qtrue : Qfalse;
}

VALUE rb_Gamepad_press(VALUE self, VALUE id, VALUE button)
{
    return raylib::IsGamepadButtonPressed(NUM2UINT(id), NUM2UINT(button)) ? Qtrue : Qfalse;
}

VALUE rb_Gamepad_release(VALUE self, VALUE id, VALUE button)
{
    return raylib::IsGamepadButtonReleased(NUM2UINT(id), NUM2UINT(button)) ? Qtrue : Qfalse;
}

VALUE rb_Gamepad_axis_position(VALUE self, VALUE id, VALUE axis)
{
    auto vaxis = NUM2LONG(axis);
    auto position = raylib::GetGamepadAxisMovement(NUM2UINT(id), vaxis);

    return DBL2NUM(static_cast<double>(position));
}

VALUE rb_Gamepad_update(VALUE self)
{
    raylib::PollInputEvents();

    return self;
}

VALUE rb_Gamepad_name(VALUE self, VALUE id)
{
    int gamepadId = NUM2UINT(id);

    return rb_utf8_str_new_cstr(raylib::GetGamepadName(gamepadId));
}

// TODO: Found why WARNING: GamepadSetVibration() not available on target platform
VALUE rb_Gamepad_set_vibration(VALUE self, VALUE id, VALUE leftMotor, VALUE rightMotor, VALUE duration)
{
    int gamepadId = NUM2UINT(id);
    float vleftMotor = (float)NUM2DBL(leftMotor);
    float vrightMotor = (float)NUM2DBL(rightMotor);
    float vduration = (float)NUM2DBL(duration);

    raylib::SetGamepadVibration(gamepadId, vleftMotor, vrightMotor, vduration);

    return self;
}

void Init_Gamepad()
{
    VALUE rb_mGamepad = rb_define_module_under(rb_mInputs, "Gamepad");

    // rb_define_const(rb_mGamepad, "COUNT", LONG2NUM(4));
    // rb_define_const(rb_mGamepad, "BUTTON_COUNT", LONG2NUM(18));
    rb_define_const(rb_mGamepad, "BUTTON_LEFT_FACE_DOWN", LONG2NUM(raylib::GAMEPAD_BUTTON_LEFT_FACE_DOWN));
    rb_define_const(rb_mGamepad, "BUTTON_LEFT_FACE_LEFT", LONG2NUM(raylib::GAMEPAD_BUTTON_LEFT_FACE_LEFT));
    rb_define_const(rb_mGamepad, "BUTTON_LEFT_FACE_RIGHT", LONG2NUM(raylib::GAMEPAD_BUTTON_LEFT_FACE_RIGHT));
    rb_define_const(rb_mGamepad, "BUTTON_LEFT_FACE_UP", LONG2NUM(raylib::GAMEPAD_BUTTON_LEFT_FACE_UP));
    rb_define_const(rb_mGamepad, "BUTTON_LEFT_THUMB", LONG2NUM(raylib::GAMEPAD_BUTTON_LEFT_THUMB));
    rb_define_const(rb_mGamepad, "BUTTON_LEFT_TRIGGER_1", LONG2NUM(raylib::GAMEPAD_BUTTON_LEFT_TRIGGER_1));
    rb_define_const(rb_mGamepad, "BUTTON_LEFT_TRIGGER_2", LONG2NUM(raylib::GAMEPAD_BUTTON_LEFT_TRIGGER_2));
    rb_define_const(rb_mGamepad, "BUTTON_MIDDLE", LONG2NUM(raylib::GAMEPAD_BUTTON_MIDDLE));
    rb_define_const(rb_mGamepad, "BUTTON_MIDDLE_LEFT", LONG2NUM(raylib::GAMEPAD_BUTTON_MIDDLE_LEFT));
    rb_define_const(rb_mGamepad, "BUTTON_MIDDLE_RIGHT", LONG2NUM(raylib::GAMEPAD_BUTTON_MIDDLE_RIGHT));
    rb_define_const(rb_mGamepad, "BUTTON_RIGHT_FACE_DOWN", LONG2NUM(raylib::GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
    rb_define_const(rb_mGamepad, "BUTTON_RIGHT_FACE_LEFT", LONG2NUM(raylib::GAMEPAD_BUTTON_RIGHT_FACE_LEFT));
    rb_define_const(rb_mGamepad, "BUTTON_RIGHT_FACE_RIGHT", LONG2NUM(raylib::GAMEPAD_BUTTON_RIGHT_FACE_RIGHT));
    rb_define_const(rb_mGamepad, "BUTTON_RIGHT_FACE_UP", LONG2NUM(raylib::GAMEPAD_BUTTON_RIGHT_FACE_UP));
    rb_define_const(rb_mGamepad, "BUTTON_RIGHT_THUMB", LONG2NUM(raylib::GAMEPAD_BUTTON_RIGHT_THUMB));
    rb_define_const(rb_mGamepad, "BUTTON_RIGHT_TRIGGER_1", LONG2NUM(raylib::GAMEPAD_BUTTON_RIGHT_TRIGGER_1));
    rb_define_const(rb_mGamepad, "BUTTON_RIGHT_TRIGGER_2", LONG2NUM(raylib::GAMEPAD_BUTTON_RIGHT_TRIGGER_2));
    rb_define_const(rb_mGamepad, "BUTTON_UNKNOWN", LONG2NUM(raylib::GAMEPAD_BUTTON_UNKNOWN));
    rb_define_const(rb_mGamepad, "AXIS_LEFT_TRIGGER", LONG2NUM(raylib::GAMEPAD_AXIS_LEFT_TRIGGER));
    rb_define_const(rb_mGamepad, "AXIS_LEFT_X", LONG2NUM(raylib::GAMEPAD_AXIS_LEFT_X));
    rb_define_const(rb_mGamepad, "AXIS_LEFT_Y", LONG2NUM(raylib::GAMEPAD_AXIS_LEFT_Y));
    rb_define_const(rb_mGamepad, "AXIS_RIGHT_TRIGGER", LONG2NUM(raylib::GAMEPAD_AXIS_RIGHT_TRIGGER));
    rb_define_const(rb_mGamepad, "AXIS_RIGHT_X", LONG2NUM(raylib::GAMEPAD_AXIS_RIGHT_X));
    rb_define_const(rb_mGamepad, "AXIS_RIGHT_Y", LONG2NUM(raylib::GAMEPAD_AXIS_RIGHT_Y));

    rb_define_module_function(rb_mGamepad, "connected?", _rbf rb_Gamepad_connected, 1);
    rb_define_module_function(rb_mGamepad, "axis_count", _rbf rb_Gamepad_axis_count, 1);
    rb_define_module_function(rb_mGamepad, "axis_available?", _rbf rb_Gamepad_axis, 2);
    rb_define_module_function(rb_mGamepad, "press?", _rbf rb_Gamepad_press, 2);
    rb_define_module_function(rb_mGamepad, "release?", _rbf rb_Gamepad_release, 2);
    rb_define_module_function(rb_mGamepad, "axis_position", _rbf rb_Gamepad_axis_position, 2);
    rb_define_module_function(rb_mGamepad, "update", _rbf rb_Gamepad_update, 0);
    rb_define_module_function(rb_mGamepad, "name", _rbf rb_Gamepad_name, 1);
    rb_define_module_function(rb_mGamepad, "set_vibration", _rbf rb_Gamepad_set_vibration, 4);
}