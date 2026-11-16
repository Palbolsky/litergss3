// Sf::Mouse — LiteRGSS2's mouse module: SFML's button values (shared with
// LiteRGSS::Inputs::Mouse) and SFML's wheel ids (0 vertical, 1 horizontal),
// which DisplayWindow#on_mouse_wheel_scrolled reports.

#include "SfMouse.h"
#include "LiteRGSS.h"
#include "RubyValue.h"

#include "LiteCGSS/Inputs/Mouse.h"

namespace {

	VALUE rb_SfMouse_press(VALUE self, VALUE button)
	{
		(void)self;
		return cgss::mouse::isButtonPressed(static_cast<cgss::MouseButton>(NUM2INT(button))) ? Qtrue : Qfalse;
	}

	VALUE rb_SfMouse_position(VALUE self)
	{
		(void)self;
		const auto position = cgss::mouse::position();
		VALUE ary = rb_ary_new_capa(2);
		rb_ary_push(ary, INT2NUM(position.x));
		rb_ary_push(ary, INT2NUM(position.y));
		return ary;
	}

	VALUE rb_SfMouse_setPosition(VALUE self, VALUE x, VALUE y)
	{
		cgss::mouse::setPosition(NUM2INT(x), NUM2INT(y));
		return self;
	}

}

void Init_SfMouse()
{
	VALUE rb_mSf = rb_define_module("Sf");
	VALUE rb_mSfMouse = rb_define_module_under(rb_mSf, "Mouse");

	const struct { const char* name; cgss::MouseButton value; } buttons[] = {
		{ "Left", cgss::MouseButton::Left },   { "LEFT", cgss::MouseButton::Left },
		{ "Right", cgss::MouseButton::Right }, { "RIGHT", cgss::MouseButton::Right },
		{ "Middle", cgss::MouseButton::Middle }, { "MIDDLE", cgss::MouseButton::Middle },
		{ "XButton1", cgss::MouseButton::XButton1 }, { "XButton2", cgss::MouseButton::XButton2 },
	};
	for (const auto& b : buttons) rb_define_const(rb_mSfMouse, b.name, INT2NUM(static_cast<int>(b.value)));
	rb_define_const(rb_mSfMouse, "VerticalWheel", INT2NUM(sf_mouse::kVerticalWheel));
	rb_define_const(rb_mSfMouse, "HorizontalWheel", INT2NUM(sf_mouse::kHorizontalWheel));

	rb_define_module_function(rb_mSfMouse, "press?", _rbf rb_SfMouse_press, 1);
	rb_define_module_function(rb_mSfMouse, "position", _rbf rb_SfMouse_position, 0);
	rb_define_module_function(rb_mSfMouse, "set_position", _rbf rb_SfMouse_setPosition, 2);
}
