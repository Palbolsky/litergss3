#ifndef Rect_H
#define Rect_H

#include "RubyValue.h"

extern VALUE rb_cRect;
void Init_Rect();

// POD value type matching the litergss2 Rect surface. Drawable-side binding
// (litergss2 used cgss::BondElement) is handled in litergss3 by per-drawable
// state, so Rect itself stays a plain value.
struct RectData
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

RectData *get_rect_data(VALUE self);

#endif
