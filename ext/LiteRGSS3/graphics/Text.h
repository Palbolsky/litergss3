#ifndef Text_H
#define Text_H

#include "RubyValue.h"
#include <LiteCGSS/Graphics/Text.h>

extern VALUE rb_cText;
void Init_Text();

// Text registers into the parent View's DrawableStack at Text.new time
// (parent View = the Ruby Viewport's cgss::Viewport when one is given,
// else the active cgss::DisplayWindow). cgss::DisplayWindow::draw() then
// renders it automatically — no per-frame Ruby draw call required.
struct TextData
{
    cgss::Text text;
    VALUE rViewport = Qnil;
    VALUE rWidth = LONG2FIX(0);
    VALUE rHeight = LONG2FIX(0);
    VALUE rText = Qnil;
    VALUE rAlign = Qfalse;
    VALUE rX = LONG2FIX(0);
    VALUE rY = LONG2FIX(0);
    bool disposed = false;
};

#endif
