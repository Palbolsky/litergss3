#ifndef SpriteMap_H
#define SpriteMap_H

#include "RubyValue.h"
#include <LiteCGSS/Graphics/SpriteMap.h>
#include <LiteCGSS/Graphics/Texture.h>

extern VALUE rb_cSpriteMap;
void Init_SpriteMap();

// SpriteMap registers into the parent View's DrawableStack at SpriteMap.new
// time. cgss::SpriteMap::setTile takes a cgss::Texture& built from the user's
// bitmap — cached here keyed by the rBitmap VALUE so repeat `set` calls
// against the same Image don't re-upload the GPU texture each call.
struct SpriteMapData
{
    cgss::SpriteMap spriteMap;
    cgss::Texture texture;
    VALUE rViewport = Qnil;
    VALUE rBitmap = Qnil;
    VALUE rX = LONG2FIX(0);
    VALUE rY = LONG2FIX(0);
    VALUE rOX = LONG2FIX(0);
    VALUE rOY = LONG2FIX(0);
    VALUE rScale = LONG2FIX(1);
    bool has_texture = false;
    bool disposed = false;
};

#endif
