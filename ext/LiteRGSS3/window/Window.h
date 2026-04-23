#ifndef Window_H
#define Window_H

#include "RubyValue.h"

#include "LiteCGSS/Backend/ActiveBackendFwd.h"

extern VALUE rb_cWindow;
void Init_Window();

extern int base_width;
extern int base_height;
extern double window_scale;

// Accessor for the process-global native window — ext/Sprite and
// ext/Viewport use this to hand a render target to Ops::draw_* calls.
// Returns nullptr if the window isn't open.
cgss::backend::ActiveBackend::native_window* get_active_native_window();

#endif