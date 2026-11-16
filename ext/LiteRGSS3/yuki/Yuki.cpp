// Yuki module — utility functions exposed via the `Yuki::` namespace.

#include "Yuki.h"
#include "LiteRGSS.h"

#include "LiteCGSS/System/Clipboard.h"

#include <string>

VALUE rb_mYuki = Qnil;

VALUE rb_Yuki_getClipboard(VALUE self)
{
    (void)self;
    const std::string text = cgss::clipboard::getString();
    if (text.empty()) return Qnil;
    return rb_utf8_str_new(text.data(), static_cast<long>(text.size()));
}

VALUE rb_Yuki_setClipboard(VALUE self, VALUE val)
{
    rb_check_type(val, T_STRING);
    // StringValueCStr raises on embedded NUL bytes, like the previous binding.
    cgss::clipboard::setString(StringValueCStr(val));
    return self;
}

void Init_Yuki()
{
    rb_mYuki = rb_define_module("Yuki");
    rb_define_module_function(rb_mYuki, "get_clipboard", _rbf rb_Yuki_getClipboard, 0);
    rb_define_module_function(rb_mYuki, "set_clipboard", _rbf rb_Yuki_setClipboard, 1);
}
