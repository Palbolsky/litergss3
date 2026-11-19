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
//
// The CPU pixels are released once uploaded when the scripts never touched
// them since the image was created or loaded (a file, a snapshot): its texture
// alone is drawn, as LiteRGSS2's Bitmap was GPU-only. Big pictures otherwise
// lived twice (Forêt Éternelle: 278 MB of pixels on Android). Any later CPU
// access (pixels()) reads them back from the texture once, and keeps them:
// an image a script edits never pays a read-back per edit.
struct ImageData
{
    cgss::Texture cachedTexture;
    bool textureValid = false;
    bool textureDirty = false;
    bool uploadQueued = false;
    bool disposed = false;

    ImageData() = default;
    ImageData(const ImageData&) = delete;
    ImageData& operator=(const ImageData&) = delete;
    ~ImageData();

    unsigned int width() const { return w; }
    unsigned int height() const { return h; }
    bool valid() const { return w > 0 && h > 0; }

    // The CPU pixels, read back from the texture if they were released. For
    // reading as for editing: from then on the image keeps them.
    cgss::Image& pixels();
    // Replaces the pixels (load, copy, dispose): the image is what they are.
    void setPixels(cgss::Image&& image);
    // The image becomes this texture, its pixels left on the GPU (snapshots)
    void adoptTexture(cgss::Texture&& texture);
    // Called once the texture holds the pixels: releases them unless a
    // script used them, or the texture's storage differs (padded).
    void releasePixelsIfUnused();
    // Diagnostics: the CPU pixels are held (not released to the texture)
    bool pixelsResident() const { return !pixelsReleased; }

private:
    // Use pixels(): released ones are empty here
    cgss::Image cpuPixels;
    unsigned int w = 0;
    unsigned int h = 0;
    bool pixelsReleased = false;
    bool pixelsUsed = false;

    friend void sync_texture(ImageData *img);
    friend cgss::Texture& image_acquire_texture(ImageData *img);
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
// the texture is the image (its pixels stay on the GPU until a script reads
// or edits them).
VALUE image_from_texture(cgss::Texture&& texture);

#endif
