#ifndef Image_H
#define Image_H

#include "RubyValue.h"
#include "Color.h"
#include <LiteCGSS/Common/NormalizeNumbers.h>
#include <LiteCGSS/Image/Image.h>
#include <LiteCGSS/Graphics/Texture.h>

void Init_Image();
extern VALUE rb_cImage;

// Ruby wrapper around cgss::Image. Backend-agnostic: loads/saves/edits
// route through cgss::Image + backend Ops, so the same file compiles
// under CGSS_BACKEND=sfml and raylib (provided the raylib-side
// ImageSerializer specializations are present, which they now are —
// see Backend/Specializations/raylib/ImageSerializer.cpp).
//
// `cachedTexture` is the lazily-uploaded GPU mirror of `image`. Sprite,
// SpriteMap, etc. share this single texture instead of each binding
// allocating its own (which produced load/unload spam every frame when
// e.g. PSDK's Tilemap rotated through autotile + main tileset bitmaps on
// the same SpriteMap row). Mirrors LiteRGSS2's design where Bitmap was
// the GPU texture directly. Mutators (fill_rect, blt, set_pixel, ...)
// flag `textureDirty`; the next `image_acquire_texture` call re-uploads.
struct ImageData
{
    cgss::Image image;
    cgss::Texture cachedTexture;
    bool textureValid = false;
    bool textureDirty = false;
    bool disposed = false;

    unsigned int width() const { return image.width(); }
    unsigned int height() const { return image.height(); }
    bool valid() const { return image.width() > 0 && image.height() > 0; }
};
ImageData *get_image(VALUE self);

// Lazily upload (or refresh) `img->cachedTexture` from the CPU pixel data
// and return a reference. Caller must NOT keep the reference past any
// mutation of the Image — call again after mutations to refresh.
cgss::Texture& image_acquire_texture(ImageData *img);

#endif
