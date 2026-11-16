#ifndef DisplayWindow_H
#define DisplayWindow_H

#include "RubyValue.h"

#include "LiteCGSS/Views/DisplayWindowFwd.h"

extern VALUE rb_cDisplayWindow;
extern VALUE rb_eClosedWindowError;

void Init_DisplayWindow();

// The window drawables join when given no DisplayWindow: the oldest one
// open (the game's). nullptr if none is open.
cgss::DisplayWindow* get_active_display_window();

// The window of `parent` when it is a DisplayWindow (nullptr if closed),
// get_active_display_window() otherwise (a Viewport / Window parent, nil).
cgss::DisplayWindow* display_window_for(VALUE parent);

#endif
