#ifndef SpriteMap_H
#define SpriteMap_H

#include "RubyValue.h"
#include <LiteCGSS/Graphics/SpriteMap.h>
#include <LiteCGSS/Views/Stack/DrawableStack.h>

extern VALUE rb_cSpriteMap;
void Init_SpriteMap();

struct SpriteMapData
{
    cgss::DrawableStack stack;
    cgss::SpriteMap spriteMap;
    VALUE rViewport = Qnil;
    VALUE rX = LONG2FIX(0);
    VALUE rY = LONG2FIX(0);
    VALUE rOX = LONG2FIX(0);
    VALUE rOY = LONG2FIX(0);
    VALUE rScale = LONG2FIX(1);
    bool disposed = false;
};

#endif
