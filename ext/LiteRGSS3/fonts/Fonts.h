#ifndef Fonts_H
#define Fonts_H

#include "RubyValue.h"

#include <LiteCGSS/Graphics/Font.h>

VALUE rb_Fonts_get_outline_color(VALUE self, VALUE id);
VALUE rb_Fonts_get_shadow_color(VALUE self, VALUE id);
VALUE rb_Fonts_get_fill_color(VALUE self, VALUE id);
VALUE rb_Fonts_get_default_size(VALUE self, VALUE id);
// Returns a reference to the backend-agnostic cgss::Font at the given
// index. Callers that legitimately need the native handle (e.g. raylib's
// DrawTextEx) can route through `cgss::extension::native(font)` via the
// opt-in extension SPI. Raises LiteRGSS::Error if the index is out of range.
cgss::Font& rb_Fonts_get_font(unsigned long id);

extern VALUE rb_mFonts;

void Init_Fonts();

#endif
