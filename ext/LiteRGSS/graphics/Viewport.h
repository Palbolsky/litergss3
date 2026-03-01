#ifndef Viewport_H
#define Viewport_H

#include "RubyValue.h"

void Init_Viewport();
extern VALUE rb_cViewport;

// Viewport data: position, size, origin offset, visibility, z-order
struct ViewportData
{
    int x      = 0;
    int y      = 0;
    int width  = 0;
    int height = 0;
    int ox     = 0;
    int oy     = 0;
    int z      = 0;
    float zoom  = 1.0f;
    float angle = 0.0f;
    bool visible = true;
    bool disposed = false;
};

#endif