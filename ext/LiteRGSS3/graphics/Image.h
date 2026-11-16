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
// route through the cgss::Image public API.
//
// `cachedTexture` is the lazily-uploaded GPU mirror of `image`. Sprite,
// SpriteMap, etc. share this single texture instead of each binding
// allocating its own (which produced load/unload spam every frame when
// e.g. PSDK's Tilemap rotated through autotile + main tileset bitmaps on
// the same SpriteMap row). Mirrors LiteRGSS2's design where Bitmap was
// the GPU texture directly. Mutators (fill_rect, blt, set_pixel, ...)
// flag `textureDirty`; the texture is re-uploaded once, either by the next
// `image_acquire_texture` call or before the next frame is drawn
// (`image_flush_pending_uploads`), so drawables already bound to it see
// the edit like LiteRGSS2's Bitmap#update.
struct ImageData
{
    cgss::Image image;
    cgss::Texture cachedTexture;
    bool textureValid = false;
    bool textureDirty = false;
    bool uploadQueued = false;
    bool disposed = false;

    ImageData() = default;
    ImageData(const ImageData&) = delete;
    ImageData& operator=(const ImageData&) = delete;
    ~ImageData();

    unsigned int width() const { return image.width(); }
    unsigned int height() const { return image.height(); }
    bool valid() const { return image.width() > 0 && image.height() > 0; }
};
ImageData *get_image(VALUE self);

// Lazily upload (or refresh) `img->cachedTexture` from the CPU pixel data
// and return a reference. Caller must NOT keep the reference past any
// mutation of the Image — call again after mutations to refresh.
cgss::Texture& image_acquire_texture(ImageData *img);

// Re-upload every image edited since its texture was last synced. Called
// before each frame is drawn, with the GVL (and so the GL context) held.
void image_flush_pending_uploads();

// Wrap a GPU texture (e.g. a viewport / window snapshot) in a new Ruby Image:
// pixels are read back once for the CPU side, and the texture itself becomes
// the image's cached GPU copy (no re-upload when it is drawn).
VALUE image_from_texture(cgss::Texture&& texture);

#endif
