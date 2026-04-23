#include "Inputs.h"
#include "RubyValue.h"

namespace {
	using Backend = cgss::backend::ActiveBackend;
	using Ops = Backend::Ops;
}

VALUE rb_Touch_count(VALUE self)
{
	(void)self;
	return UINT2NUM(Ops::touch_count());
}

VALUE rb_Touch_getPosition(VALUE self, VALUE index)
{
	(void)self;
	const auto position = Ops::touch_position(NUM2UINT(index));
	VALUE ary = rb_ary_new_capa(2);
	rb_ary_push(ary, INT2NUM(position.x));
	rb_ary_push(ary, INT2NUM(position.y));
	return ary;
}

VALUE rb_Touch_getId(VALUE self, VALUE index)
{
	(void)self;
	return UINT2NUM(Ops::touch_id(NUM2UINT(index)));
}

void Init_Touch()
{
	// Only register the LiteRGSS::Inputs::Touch module under backends that
	// support touch polling. Referencing it under SFML (has_input_touch=false)
	// gives a clean NameError in Ruby rather than a silently-broken API.
	if constexpr (Backend::has_input_touch) {
		VALUE rb_mTouch = rb_define_module_under(rb_mInputs, "Touch");

		rb_define_module_function(rb_mTouch, "count",    _rbf rb_Touch_count,       0);
		rb_define_module_function(rb_mTouch, "position", _rbf rb_Touch_getPosition, 1);
		rb_define_module_function(rb_mTouch, "id",       _rbf rb_Touch_getId,       1);
	}
}
