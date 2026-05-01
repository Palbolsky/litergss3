#ifndef FramedView_H
#define FramedView_H

#include "RubyValue.h"
#include "../rbAdapter.h"

extern VALUE rb_cFramedView;
void Init_FramedView();

// LiteRGSS::Window — the FramedView-based UI widget (per-game-menu frame).
// Distinct from LiteRGSS::DisplayWindow (the host OS window).
//
// Implementation note: cgss::FramedView requires MainEventDispatcher +
// DisplayWindowUserData + DisplayWindowSettings, which litergss3's
// DisplayWindow doesn't currently expose. This port stores the widget's
// state on the Ruby side (skin, cursor rect, opacity, geometry) so the
// API surface matches litergss2's Window — full cgss::FramedView wiring
// folds in once DisplayWindow exposes the cgss event/window context.
struct FramedViewData
{
    int x = 0, y = 0, width = 0, height = 0;
    int ox = 0, oy = 0;
    int z = 0;
    int pause_x = 0, pause_y = 0;
    uint8_t opacity = 255;
    uint8_t back_opacity = 255;
    uint8_t contents_opacity = 255;
    bool active = false;
    bool paused = false;
    bool stretch = false;
    bool visible = true;
    bool locked = false;
    bool disposed = false;

    VALUE rViewport = Qnil;
    VALUE rWindowskin = Qnil;
    VALUE rCursorskin = Qnil;
    VALUE rPauseskin = Qnil;
    VALUE rWindowBuilder = Qnil;
    VALUE rCursorRect = Qnil;
};

// Forward-declare the Mark specialization so every TU that instantiates
// rb::GetDataType<FramedViewData> binds the same out-of-line symbol.
namespace rb {
    template <> void Mark<FramedViewData>(void *ptr);
}

#endif
