#ifndef Viewport_H
#define Viewport_H

#include "RubyValue.h"

void Init_Viewport();
extern VALUE rb_cViewport;

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
};

ViewportData* get_viewport(VALUE self);

#endif
