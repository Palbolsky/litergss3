#include "Inputs.h"
#include "RubyValue.h"

#include "LiteCGSS/Events/MouseButton.h"

namespace {
	using Backend = cgss::backend::ActiveBackend;
	using Ops = Backend::Ops;
}

VALUE rb_Mouse_press(VALUE self, VALUE button)
{
	(void)self;
	return Ops::mouse_is_button_pressed(static_cast<cgss::MouseButton>(NUM2INT(button))) ? Qtrue : Qfalse;
}

VALUE rb_Mouse_getPosition(VALUE self)
{
	(void)self;
	const auto position = Ops::mouse_get_position();
	VALUE ary = rb_ary_new_capa(2);
	rb_ary_push(ary, INT2NUM(position.x));
	rb_ary_push(ary, INT2NUM(position.y));
	return ary;
}

VALUE rb_Mouse_setPosition(VALUE self, VALUE x, VALUE y)
{
	Ops::mouse_set_position(NUM2INT(x), NUM2INT(y));
	return self;
}

VALUE rb_Mouse_wheelMove(VALUE self)
{
	(void)self;
	// SFML reports wheel only via events (has_wheel_state_query=false);
	// Raylib has a proper sync query. Under SFML, Ops::mouse_wheel_delta()
	// returns 0.0 — PSDK scripts that want wheel under SFML must poll events.
	if constexpr (Backend::has_wheel_state_query) {
		return DBL2NUM(Ops::mouse_wheel_delta());
	} else {
		return DBL2NUM(0.0);
	}
}

void Init_Mouse()
{
	VALUE rb_mMouse = rb_define_module_under(rb_mInputs, "Mouse");

	// Constants sourced from cgss::MouseButton — same numeric layout under
	// both backends (see MouseButton.h).
#define X(Name) \
	rb_define_const(rb_mMouse, #Name, INT2NUM(static_cast<int>(cgss::MouseButton::Name)));
	CGSS_MOUSEBUTTON_LIST(X)
#undef X
	// XButton3 was present in the previous (raylib-direct) binding but has
	// no cgss::MouseButton equivalent (it mapped to raylib MOUSE_BUTTON_FORWARD=5).
	// Emit it as the raw backend value for backwards compat.
	rb_define_const(rb_mMouse, "XButton3", INT2NUM(5));

	rb_define_module_function(rb_mMouse, "press?",        _rbf rb_Mouse_press,       1);
	rb_define_module_function(rb_mMouse, "position",      _rbf rb_Mouse_getPosition, 0);
	rb_define_module_function(rb_mMouse, "set_position",  _rbf rb_Mouse_setPosition, 2);
	rb_define_module_function(rb_mMouse, "wheel_move",    _rbf rb_Mouse_wheelMove,   0);
}
