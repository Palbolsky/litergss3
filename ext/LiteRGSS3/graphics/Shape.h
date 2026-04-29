#ifndef Shape_H
#define Shape_H

#include "RubyValue.h"
#include <LiteCGSS/Graphics/Shape.h>
#include <LiteCGSS/Graphics/ShapeGeometry.h>
#include <LiteCGSS/Views/Stack/DrawableStack.h>

extern VALUE rb_cShape;
void Init_Shape();

struct ShapeData
{
    cgss::DrawableStack stack;
    cgss::Shape shape;
    VALUE rViewport = Qnil;
    VALUE rBitmap = Qnil;
    VALUE rX = LONG2FIX(0);
    VALUE rY = LONG2FIX(0);
    VALUE rOX = LONG2FIX(0);
    VALUE rOY = LONG2FIX(0);
    VALUE rAngle = LONG2FIX(0);
    VALUE rZoomX = LONG2FIX(1);
    VALUE rZoomY = LONG2FIX(1);
    VALUE rShapeType = Qnil;
    VALUE rBlendMode = Qnil;
    bool disposed = false;
};

#endif
