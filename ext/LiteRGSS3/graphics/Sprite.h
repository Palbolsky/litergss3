#ifndef Sprite_H
#define Sprite_H

#include "RubyValue.h"
#include "../rbAdapter.h"

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
    // Cached Rect returned by `sprite.src_rect`. Lazily allocated on first
    // read; mutations (rect.set / rect.x= etc.) propagate back to this
    // sprite via the Rect's on_change hook (see Rect.h). A standalone
    // RectData (without on_change) wouldn't survive PSDK's idiom of
    // `sprite.src_rect.set(0, 0, 32, 32)` — Ruby returns the Rect, then
    // mutates it, so the change has to flow through the Rect itself.
    VALUE rRect = Qnil;

    // cgss::basic_SpriteItem holds a raw `m_transformable` pointer to the
    // owning cgss::Sprite. The wrapper MUST outlive its registration in the
    // parent View's DrawableStack — when Ruby GC reclaims this struct, we
    // first detach the StackItem so cgss stops dereferencing the wrapper.
    ~SpriteData() {
        if (sprite && !disposed) sprite->detach();
    }
};

// Mark hook MUST be declared in the header so every TU that instantiates
// rb::GetDataType<SpriteData> (Sprite.cpp AND ShaderSprite.cpp) wires the
// same specialization into the static rb_data_type_t. The body lives in
// Sprite.cpp — one out-of-line symbol shared by all callers.
namespace rb {
    template <> void Mark<SpriteData>(void *ptr);
}

#endif
