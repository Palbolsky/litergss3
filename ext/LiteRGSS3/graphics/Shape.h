#ifndef Shape_H
#define Shape_H

#include "RubyValue.h"
#include "../rbAdapter.h"
#include <LiteCGSS/Graphics/Shape.h>
#include <LiteCGSS/Graphics/ShapeGeometry.h>

extern VALUE rb_cShape;
void Init_Shape();

// Shape registers into the parent View's DrawableStack at Shape.new time —
// see TextData for the same model.
struct ShapeData
{
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

    ~ShapeData() {
        if (!disposed) shape.detach();
    }
};

// Forward-declare the Mark specialization so every TU that instantiates
// rb::GetDataType<ShapeData> binds the same out-of-line symbol.
namespace rb {
    template <> void Mark<ShapeData>(void *ptr);
}

#endif
