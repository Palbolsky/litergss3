#ifndef FramedView_H
#define FramedView_H

#include "RubyValue.h"
#include "../rbAdapter.h"

#include <LiteCGSS/Views/FramedView.h>
#include <memory>

extern VALUE rb_cFramedView;
void Init_FramedView();

// LiteRGSS::Window — backs PSDK's per-game-menu/dialog window. Wraps a
// cgss::FramedView registered into either a parent cgss::Viewport or the
// active cgss::DisplayWindow at construction time. Sprite/Text/Shape
// instantiated with a Window as parent register into THIS FramedView's
// child stack — that's how the dialog text gets offset to the box's
// position and clipped to the box's bounds.
//
// Distinct from LiteRGSS::DisplayWindow (the host OS window).
struct FramedViewData
{
    // The cgss FramedView. Move-assigned in initialize once the parent
    // (Viewport or DisplayWindow) is known. has_view gates forwarding.
    std::unique_ptr<cgss::FramedView> view;
    bool has_view = false;
    bool disposed = false;

    // Cached Ruby-side state used as the source of truth for getters and
    // for round-tripping back through Ruby (mirrors litergss2's
    // FramedViewElement). Width/height also fall back to these for getters
    // because cgss::FramedView only exposes float getters.
    int x = 0, y = 0, width = 0, height = 0;
    int ox = 0, oy = 0;

    VALUE rViewport = Qnil;
    VALUE rWindowskin = Qnil;
    VALUE rCursorskin = Qnil;
    VALUE rPauseskin = Qnil;
    VALUE rWindowBuilder = Qnil;
    VALUE rCursorRect = Qnil;

    ~FramedViewData() {
        if (view && !disposed) view->detach();
    }
};

// Forward-declare the Mark specialization so every TU that instantiates
// rb::GetDataType<FramedViewData> binds the same out-of-line symbol.
namespace rb {
    template <> void Mark<FramedViewData>(void *ptr);
}

FramedViewData* get_framed_view(VALUE self);

#endif
