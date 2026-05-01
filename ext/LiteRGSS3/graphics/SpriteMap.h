#ifndef SpriteMap_H
#define SpriteMap_H

#include "RubyValue.h"
#include <LiteCGSS/Graphics/SpriteMap.h>

extern VALUE rb_cSpriteMap;
void Init_SpriteMap();

// SpriteMap registers into the parent View's DrawableStack at SpriteMap.new
// time. setTile is fed the GPU texture cached on the source Image (see
// Image.h's cachedTexture); cgss::SpriteMap takes a shared_ptr ref on the
// native handle internally, so this struct doesn't need to track textures.
struct SpriteMapData
{
    cgss::SpriteMap spriteMap;
    VALUE rViewport = Qnil;
    VALUE rX = LONG2FIX(0);
    VALUE rY = LONG2FIX(0);
    VALUE rOX = LONG2FIX(0);
    VALUE rOY = LONG2FIX(0);
    VALUE rScale = LONG2FIX(1);
    bool disposed = false;

    ~SpriteMapData() {
        if (!disposed) spriteMap.detach();
    }
};

#endif
