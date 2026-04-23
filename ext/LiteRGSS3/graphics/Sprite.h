#ifndef Sprite_H
#define Sprite_H

#include "RubyValue.h"
#include <LiteCGSS2/Common/RaylibWrapper.h>
#include <LiteCGSS/Common/NormalizeNumbers.h>

void Init_Sprite();
extern VALUE rb_cSprite;

/*namespace meta
{
    template <>
    struct Log<Sprite>
    {
        static constexpr auto classname = "Sprite";
    };
}*/

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

    raylib::Texture2D *texture = nullptr; // not owned
    int src_x = 0;
    int src_y = 0;
    int src_width = 0;
    int src_height = 0;

    VALUE rBitmap = Qnil;
    VALUE rViewport = Qnil;
};

#endif