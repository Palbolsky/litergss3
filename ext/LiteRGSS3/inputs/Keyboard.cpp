#include "Inputs.h"
#include "RubyValue.h"

#include "LiteCGSS/Events/KeyCode.h"

namespace {
	using Ops = cgss::backend::ActiveBackend::Ops;
}

VALUE rb_Keyboard_press(VALUE self, VALUE key)
{
	(void)self;
	return Ops::keyboard_is_key_pressed(static_cast<cgss::KeyCode>(NUM2INT(key))) ? Qtrue : Qfalse;
}

static void DefineKeyboardConstants(VALUE rb_mKeyboard)
{
	// Canonical cgss::KeyCode names (SFML 2.6 layout) — single source of
	// truth for values. See external/litecgss2/include/LiteCGSS/Events/KeyCode.h.
#define X(Name) \
	rb_define_const(rb_mKeyboard, #Name, INT2NUM(static_cast<int>(cgss::KeyCode::Name)));
	CGSS_KEYCODE_LIST(X)
#undef X

	// Raylib-style alias names (Zero, One, Minus, LSuper, ...) retained so
	// existing PSDK scripts continue to resolve. Maps to the canonical enum
	// value, no duplicate constants.
#define X(AliasName, Canonical) \
	rb_define_const(rb_mKeyboard, #AliasName, INT2NUM(static_cast<int>(cgss::KeyCode::Canonical)));
	CGSS_KEYCODE_RAYLIB_ALIAS_LIST(X)
#undef X
}

void Init_Keyboard()
{
	VALUE rb_mKeyboard = rb_define_module_under(rb_mInputs, "Keyboard");

	DefineKeyboardConstants(rb_mKeyboard);

	rb_define_module_function(rb_mKeyboard, "press?", _rbf rb_Keyboard_press, 1);
}
