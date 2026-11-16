#ifndef Fonts_H
#define Fonts_H

#include "RubyValue.h"

#include <LiteCGSS/Graphics/Font.h>

VALUE rb_Fonts_get_outline_color(VALUE self, VALUE id);
VALUE rb_Fonts_get_shadow_color(VALUE self, VALUE id);
VALUE rb_Fonts_get_fill_color(VALUE self, VALUE id);
VALUE rb_Fonts_get_default_size(VALUE self, VALUE id);
// Returns a reference to the backend-agnostic cgss::Font at the given
// index. Raises LiteRGSS::Error if the index is out of range.
cgss::Font& rb_Fonts_get_font(unsigned long id);

extern VALUE rb_mFonts;

void Init_Fonts();

#endif
