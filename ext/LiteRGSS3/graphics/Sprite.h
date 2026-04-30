#ifndef Sprite_H
#define Sprite_H

#include "RubyValue.h"

#include <LiteCGSS/Common/IntRect.h>
#include <LiteCGSS/Common/NormalizeNumbers.h>
#include <LiteCGSS/Graphics/Sprite.h>
#include <LiteCGSS/Graphics/Texture.h>
#include <memory>

void Init_Sprite();
extern VALUE rb_cSprite;

// Ruby Sprite data. The cgss::Sprite is registered into the parent View's
// DrawableStack at Sprite.new time and drawn automatically by
// cgss::DisplayWindow::draw(). Ruby setters forward to the cgss::Sprite
// proxy (transform, color, texture, mirror) — no manual per-frame draw.
struct SpriteData
{
    // Cached Ruby-side state. Mirrors what the cgss::Sprite holds so reads
    // round-trip without going back through the backend.
    float x = 0.0f;
    float y = 0.0f;
    float ox = 0.0f;
    float oy = 0.0f;

    float angle = 0.0f;
    float zoom_x = 1.0f;
    float zoom_y = 1.0f;

    bool visible = true;
    bool mirror = false;
    bool disposed = false;
    uint8_t opacity = 255;

    int z = 0;
    int src_x = 0;
    int src_y = 0;
    int src_width = 0;
    int src_height = 0;
    bool src_rect_user_set = false;

    // GPU-resident texture. Empty (default-constructed) means "no bitmap
    // assigned" — has_texture tells the bind path whether a bitmap exists.
    cgss::Texture texture;
    bool has_texture = false;

    // The actual cgss::Sprite, lazily move-assigned in initialize after the
    // parent View is known. has_sprite gates all forwarding setters.
    std::unique_ptr<cgss::Sprite> sprite;
    bool has_sprite = false;

    VALUE rBitmap = Qnil;
    VALUE rViewport = Qnil;
};

#endif
