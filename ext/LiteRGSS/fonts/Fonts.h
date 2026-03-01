#ifndef Fonts_H
#define Fonts_H

#include "RubyValue.h"
#include "RaylibWrapper.h"

VALUE rb_Fonts_get_outline_color(VALUE self, VALUE id);
VALUE rb_Fonts_get_shadow_color(VALUE self, VALUE id);
VALUE rb_Fonts_get_fill_color(VALUE self, VALUE id);
VALUE rb_Fonts_get_default_size(VALUE self, VALUE id);
raylib::Font &rb_Fonts_get_font(unsigned long id);

extern VALUE rb_mFonts;

void Init_Fonts();

#endif