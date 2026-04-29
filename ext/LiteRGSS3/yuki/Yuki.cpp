// Yuki module — utility functions exposed via the `Yuki::` namespace.
//
// Clipboard limitation: cgss::backend::ActiveBackend::Ops does not expose
// clipboard_* (verified via grep across external/litecgss2/include). Routing
// through Ops would be cleaner, but lacking that, fall back to direct
// backend calls behind a CGSS_BACKEND_* macro guard. TODO: collapse onto
// Ops::clipboard_get_string / set_string once LiteCGSS2 exposes them.

#include "Yuki.h"
#include "LiteRGSS.h"
#include <string>

#if defined(CGSS_BACKEND_SFML)
    #include <SFML/Window/Clipboard.hpp>
    #include <SFML/System/String.hpp>
#elif defined(CGSS_BACKEND_RAYLIB)
    #include <raylib.h>
#endif

VALUE rb_mYuki = Qnil;

VALUE rb_Yuki_getClipboard(VALUE self)
{
    (void)self;
#if defined(CGSS_BACKEND_SFML)
    sf::String s = sf::Clipboard::getString();
    if (s.getSize() == 0) return Qnil;
    auto bytes = s.toUtf8();
    return rb_utf8_str_new_cstr(reinterpret_cast<const char *>(bytes.c_str()));
#elif defined(CGSS_BACKEND_RAYLIB)
    const char *txt = GetClipboardText();
    if (txt == nullptr || *txt == '\0') return Qnil;
    return rb_utf8_str_new_cstr(txt);
#else
    return Qnil;
#endif
}

VALUE rb_Yuki_setClipboard(VALUE self, VALUE val)
{
    rb_check_type(val, T_STRING);
#if defined(CGSS_BACKEND_SFML)
    std::string utf8(RSTRING_PTR(val), RSTRING_LEN(val));
    sf::Clipboard::setString(sf::String::fromUtf8(utf8.begin(), utf8.end()));
#elif defined(CGSS_BACKEND_RAYLIB)
    SetClipboardText(StringValueCStr(val));
#endif
    return self;
}

void Init_Yuki()
{
    rb_mYuki = rb_define_module("Yuki");
    rb_define_module_function(rb_mYuki, "get_clipboard", _rbf rb_Yuki_getClipboard, 0);
    rb_define_module_function(rb_mYuki, "set_clipboard", _rbf rb_Yuki_setClipboard, 1);
}
