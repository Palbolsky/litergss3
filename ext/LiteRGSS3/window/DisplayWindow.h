#ifndef DisplayWindow_H
#define DisplayWindow_H

#include "RubyValue.h"

#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Views/DisplayWindowFwd.h"

extern VALUE rb_cDisplayWindow;
extern VALUE rb_eClosedWindowError;

void Init_DisplayWindow();

extern int base_width;
extern int base_height;
extern double window_scale;

// Accessor for the process-global cgss::DisplayWindow. Sprite/Viewport/etc.
// register into its View (or a child Viewport's View) via cgss::*::create.
// Returns nullptr if the window isn't open.
cgss::DisplayWindow* get_active_display_window();

#endif
