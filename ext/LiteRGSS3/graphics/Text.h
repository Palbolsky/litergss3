#ifndef Text_H
#define Text_H

#include "RubyValue.h"
#include <LiteCGSS/Graphics/Text.h>
#include <LiteCGSS/Views/Stack/DrawableStack.h>

extern VALUE rb_cText;
void Init_Text();

// Litergss3 Text owns a private DrawableStack so the cgss::Text can be
// constructed via the create(stack) factory without requiring host-side
// stack integration (the way litergss2 had — via Viewport.initAndAdd).
// Limitation: viewport-relative origin/zoom/angle aren't applied on top
// of the text's own transform yet — that requires viewport stack ownership,
// tracked as follow-up.
struct TextData
{
    cgss::DrawableStack stack;
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
