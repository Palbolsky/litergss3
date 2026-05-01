#ifndef Rect_H
#define Rect_H

#include "RubyValue.h"

extern VALUE rb_cRect;
void Init_Rect();

// POD value type matching the litergss2 Rect surface. Drawable-side binding
// is folded directly into RectData via the on_change callback below — when
// a Rect is bound to a drawable (e.g. Sprite#src_rect), Ruby mutations like
// `rect.set(x, y, w, h)` or `rect.x = ...` invoke the owner's callback so
// the drawable can re-apply its native state. Without this hook, the
// `sprite.src_rect.set(...)` idiom (Ruby returns the Rect, then mutates it)
// would never reach the sprite — the sprite would keep its old rect and
// PSDK's character spritesheets would render full-sheet instead of a frame.
struct RectData
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    // Drawable-binding hook. `owner` is the Ruby drawable VALUE that owns
    // this Rect (or Qnil for standalone Rects). `on_change` is invoked
    // after every mutation (set / set_x= / set_y= / set_width= / set_height=)
    // with `owner` as its argument, letting the drawable forward the new
    // rect into its native state. nullptr → no-op (standalone Rect).
    void (*on_change)(VALUE owner) = nullptr;
    VALUE owner = Qnil;
};

RectData *get_rect_data(VALUE self);

#endif
