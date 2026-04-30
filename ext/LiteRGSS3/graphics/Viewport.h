#ifndef Viewport_H
#define Viewport_H

#include "RubyValue.h"

#include <LiteCGSS/Views/Viewport.h>
#include <memory>

void Init_Viewport();
extern VALUE rb_cViewport;

// Ruby Viewport wraps cgss::Viewport. The native viewport is registered
// into the active cgss::DisplayWindow's DrawableStack at construction time
// — Sprite/Text/Shape/SpriteMap created with this viewport register into
// THIS viewport's stack, not the window's, so they get its scissor and
// transform during cgss::DisplayWindow::draw().
struct ViewportData
{
    int x      = 0;
    int y      = 0;
    int width  = 0;
    int height = 0;
    int ox     = 0;
    int oy     = 0;
    int z      = 0;
    // Monotonic creation index. PSDK's Graphics module uses this to order
    // viewports that share the same z (LiteRGSS2: __index__).
    unsigned long index = 0;
    float zoom  = 1.0f;
    float angle = 0.0f;
    bool visible = true;
    bool disposed = false;

    // Lazily move-assigned in initialize once the active DisplayWindow is
    // available. Wrapped in unique_ptr because cgss::Viewport is non-copyable
    // and needs deferred construction.
    std::unique_ptr<cgss::Viewport> viewport;
    bool has_viewport = false;
};

ViewportData* get_viewport(VALUE self);

#endif
