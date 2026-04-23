#ifndef Sprite_H
#define Sprite_H

#include "RubyValue.h"

#include <LiteCGSS/Common/IntRect.h>
#include <LiteCGSS/Common/NormalizeNumbers.h>
#include <LiteCGSS/Graphics/Texture.h>

void Init_Sprite();
extern VALUE rb_cSprite;

// Ruby Sprite data. All rendering state is cross-backend (cgss::Texture
// for the GPU backing; plain floats for transform). The per-frame draw()
// routes through Ops::draw_texture_pro — raylib: DrawTexturePro;
// SFML: emulated via sf::Sprite on the active render target.
struct SpriteData
{
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

    // GPU-resident texture. Empty (default-constructed) means "no bitmap
    // assigned" — `has_texture` tells draw() whether to render.
    cgss::Texture texture;
    bool has_texture = false;
    int src_x = 0;
    int src_y = 0;
    int src_width = 0;
    int src_height = 0;

    VALUE rBitmap = Qnil;
    VALUE rViewport = Qnil;
};

#endif
