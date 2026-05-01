// Ruby Shape binding. The cgss::Shape registers into the parent View's
// DrawableStack at Shape.new time; cgss::DisplayWindow::draw() iterates and
// renders it. Setters forward to the cgss::Shape proxy.

#include "LiteRGSS.h"
#include "RubyValue.h"
#include "../rbAdapter.h"
#include <LiteCGSS/Views/DisplayWindow.h>
#include <LiteCGSS/Views/Viewport.h>
#include <cmath>
#include "Shape.h"
#include "Color.h"
#include "DrawableDisposable.h"
#include "DisplayWindow.h"
#include "Image.h"
#include "Viewport.h"
#include "window/FramedView.h"

VALUE rb_cShape = Qnil;
static ID rb_iShapeCircle = 0;
static ID rb_iShapeConvex = 0;
static ID rb_iShapeRectangle = 0;

namespace rb {
    template <>
    void Mark<ShapeData>(void *ptr)
    {
        auto *s = static_cast<ShapeData *>(ptr);
        if (s == nullptr) return;
        rb_gc_mark(s->rViewport);
        rb_gc_mark(s->rBitmap);
        rb_gc_mark(s->rX);
        rb_gc_mark(s->rY);
        rb_gc_mark(s->rOX);
        rb_gc_mark(s->rOY);
        rb_gc_mark(s->rAngle);
        rb_gc_mark(s->rZoomX);
        rb_gc_mark(s->rZoomY);
        rb_gc_mark(s->rShapeType);
        rb_gc_mark(s->rBlendMode);
    }
}

static ShapeData *get_shape(VALUE self) { return rb::GetPtr<ShapeData>(self); }

VALUE rb_Shape_Initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE viewport, type, rad_numPoint, numPoint;
    rb_scan_args(argc, argv, "22", &viewport, &type, &rad_numPoint, &numPoint);

    auto *s = get_shape(self);
    s->rViewport = viewport;

    cgss::ShapeGeometry geometry{};
    ID itype = SYM2ID(type);

    if (itype == rb_iShapeCircle) {
        // (radius, point_count) or (point_count) with default radius
        if (NIL_P(numPoint)) {
            geometry.pointCount = NUM2ULONG(rad_numPoint);
            geometry.radius = 1.0f;
        } else {
            if (!NIL_P(rad_numPoint)) geometry.radius = std::abs(static_cast<float>(NUM2DBL(rad_numPoint)));
            geometry.pointCount = NUM2ULONG(numPoint);
        }
        geometry.type = cgss::ShapeType::Circle;
        s->rShapeType = type;
    } else if (itype == rb_iShapeConvex) {
        std::size_t numpt = 4;
        if (!NIL_P(rad_numPoint)) numpt = NUM2ULONG(rad_numPoint);
        geometry.pointCount = numpt;
        geometry.points.resize(numpt);
        geometry.type = cgss::ShapeType::Convex;
        s->rShapeType = type;
    } else {
        // Rectangle (default if symbol unknown)
        if (!NIL_P(rad_numPoint) && !NIL_P(numPoint)) {
            geometry.width = static_cast<float>(NUM2DBL(rad_numPoint));
            geometry.height = static_cast<float>(NUM2DBL(numPoint));
        }
        geometry.type = cgss::ShapeType::Rectangle;
        s->rShapeType = ID2SYM(rb_iShapeRectangle);
    }

    auto* window = get_active_display_window();
    if (window == nullptr) {
        rb_raise(rb_eRGSSError, "Shape.new requires an open DisplayWindow");
    }
    // LiteRGSS2 accepted Viewport, Window (FramedView) or DisplayWindow.
    const bool is_viewport = RTEST(viewport) && rb_obj_is_kind_of(viewport, rb_cViewport) == Qtrue;
    const bool is_framed   = !is_viewport && RTEST(viewport) &&
                             rb_obj_is_kind_of(viewport, rb_cFramedView) == Qtrue;
    if (is_viewport) {
        auto *vp = get_viewport(viewport);
        if (vp == nullptr || !vp->viewport) {
            rb_raise(rb_eRGSSError, "Shape.new viewport is not initialized");
        }
        s->shape = cgss::Shape::create(*vp->viewport, std::move(geometry));
    } else if (is_framed) {
        auto *fv = get_framed_view(viewport);
        if (fv == nullptr || !fv->has_view) {
            rb_raise(rb_eRGSSError, "Shape.new Window parent is not initialized");
        }
        s->shape = cgss::Shape::create(*fv->view, std::move(geometry));
    } else {
        s->shape = cgss::Shape::create(*window, std::move(geometry));
    }
    if (!is_viewport && !is_framed) s->rViewport = Qnil;
    return self;
}

VALUE rb_Shape_Dispose(VALUE self)
{
    auto *s = get_shape(self);
    if (!s->disposed) { s->shape.detach(); s->disposed = true; }
    return self;
}
VALUE rb_Shape_Disposed(VALUE self) { return get_shape(self)->disposed ? Qtrue : Qfalse; }
VALUE rb_Shape_Copy(VALUE self) { (void)self; rb_raise(rb_eRGSSError, "Shapes cannot be cloned or duplicated."); return self; }

VALUE rb_Shape_getX(VALUE self) { return get_shape(self)->rX; }
VALUE rb_Shape_setX(VALUE self, VALUE val)
{
    auto *s = get_shape(self);
    s->shape.move(static_cast<float>(NUM2DBL(val)), static_cast<float>(NUM2DBL(s->rY)));
    s->rX = val; return val;
}
VALUE rb_Shape_getY(VALUE self) { return get_shape(self)->rY; }
VALUE rb_Shape_setY(VALUE self, VALUE val)
{
    auto *s = get_shape(self);
    s->shape.move(static_cast<float>(NUM2DBL(s->rX)), static_cast<float>(NUM2DBL(val)));
    s->rY = val; return val;
}
VALUE rb_Shape_setPosition(VALUE self, VALUE x, VALUE y)
{
    auto *s = get_shape(self);
    s->shape.move(static_cast<float>(NUM2DBL(x)), static_cast<float>(NUM2DBL(y)));
    s->rX = x; s->rY = y; return self;
}

VALUE rb_Shape_getZ(VALUE self) { return LONG2NUM(get_shape(self)->shape.getZ().z); }
VALUE rb_Shape_setZ(VALUE self, VALUE val) { get_shape(self)->shape.setZ(NUM2LONG(val)); return val; }
VALUE rb_Shape_Index(VALUE self) { return LONG2NUM(get_shape(self)->shape.getZ().index); }

VALUE rb_Shape_getOX(VALUE self) { return get_shape(self)->rOX; }
VALUE rb_Shape_setOX(VALUE self, VALUE val)
{
    auto *s = get_shape(self);
    s->shape.moveOrigin(static_cast<float>(NUM2DBL(val)), static_cast<float>(NUM2DBL(s->rOY)));
    s->rOX = val; return val;
}
VALUE rb_Shape_getOY(VALUE self) { return get_shape(self)->rOY; }
VALUE rb_Shape_setOY(VALUE self, VALUE val)
{
    auto *s = get_shape(self);
    s->shape.moveOrigin(static_cast<float>(NUM2DBL(s->rOX)), static_cast<float>(NUM2DBL(val)));
    s->rOY = val; return val;
}
VALUE rb_Shape_setOrigin(VALUE self, VALUE ox, VALUE oy)
{
    auto *s = get_shape(self);
    s->shape.moveOrigin(static_cast<float>(NUM2DBL(ox)), static_cast<float>(NUM2DBL(oy)));
    s->rOX = ox; s->rOY = oy; return self;
}

VALUE rb_Shape_getAngle(VALUE self) { return get_shape(self)->rAngle; }
VALUE rb_Shape_setAngle(VALUE self, VALUE val)
{
    auto *s = get_shape(self);
    float angle = static_cast<float>(NUM2DBL(val));
    if (angle >= 360.0f || angle < 0.0f) {
        angle = std::fmod(angle, 360.0f);
        s->rAngle = (angle == static_cast<long>(angle)) ? LONG2NUM(static_cast<long>(angle)) : DBL2NUM(angle);
    } else s->rAngle = val;
    s->shape.setAngle(angle);
    return val;
}

VALUE rb_Shape_getZoomX(VALUE self) { return get_shape(self)->rZoomX; }
VALUE rb_Shape_setZoomX(VALUE self, VALUE val)
{
    auto *s = get_shape(self);
    s->shape.scale(static_cast<float>(NUM2DBL(val)), static_cast<float>(NUM2DBL(s->rZoomY)));
    s->rZoomX = val; return val;
}
VALUE rb_Shape_getZoomY(VALUE self) { return get_shape(self)->rZoomY; }
VALUE rb_Shape_setZoomY(VALUE self, VALUE val)
{
    auto *s = get_shape(self);
    s->shape.scale(static_cast<float>(NUM2DBL(s->rZoomX)), static_cast<float>(NUM2DBL(val)));
    s->rZoomY = val; return val;
}
VALUE rb_Shape_setZoom(VALUE self, VALUE val)
{
    auto *s = get_shape(self);
    float z = static_cast<float>(NUM2DBL(val));
    s->shape.scale(z, z);
    s->rZoomX = val; s->rZoomY = val; return val;
}

VALUE rb_Shape_getViewport(VALUE self) { return get_shape(self)->rViewport; }
VALUE rb_Shape_getVisible(VALUE self) { return get_shape(self)->shape.isVisible() ? Qtrue : Qfalse; }
VALUE rb_Shape_setVisible(VALUE self, VALUE val) { get_shape(self)->shape.setVisible(RTEST(val)); return val; }

VALUE rb_Shape_getPointCount(VALUE self) { return ULONG2NUM(get_shape(self)->shape.getPointCount()); }
VALUE rb_Shape_setPointCount(VALUE self, VALUE val) { get_shape(self)->shape.setPointCount(NUM2ULONG(val)); return val; }
VALUE rb_Shape_getRadius(VALUE self) { return DBL2NUM(get_shape(self)->shape.getRadius()); }
VALUE rb_Shape_setRadius(VALUE self, VALUE val) { get_shape(self)->shape.setRadius(static_cast<float>(NUM2DBL(val))); return val; }
VALUE rb_Shape_getType(VALUE self) { return get_shape(self)->rShapeType; }

VALUE rb_Shape_getPoint(VALUE self, VALUE index)
{
    auto *s = get_shape(self);
    long i = NUM2LONG(index);
    if (i < 0 || static_cast<std::size_t>(i) >= s->shape.getPointCount()) return Qnil;
    auto p = s->shape.getPoint(i);
    VALUE arr = rb_ary_new();
    rb_ary_push(arr, DBL2NUM(p.x));
    rb_ary_push(arr, DBL2NUM(p.y));
    return arr;
}
VALUE rb_Shape_setPoint(VALUE self, VALUE index, VALUE x, VALUE y)
{
    get_shape(self)->shape.setPoint(NUM2LONG(index),
                                    static_cast<float>(NUM2DBL(x)),
                                    static_cast<float>(NUM2DBL(y)));
    return self;
}

static VALUE color_to_rb(cgss::Color c)
{
    VALUE argv[4] = { INT2NUM(c.r), INT2NUM(c.g), INT2NUM(c.b), INT2NUM(c.a) };
    return rb_class_new_instance(4, argv, rb_cColor);
}

VALUE rb_Shape_getColor(VALUE self) { return color_to_rb(get_shape(self)->shape.getFillColor()); }
VALUE rb_Shape_setColor(VALUE self, VALUE val)
{
    if (rb_obj_is_kind_of(val, rb_cColor) != Qtrue) return val;
    auto *cd = get_color_data(val);
    get_shape(self)->shape.setFillColor(cgss::Color{cd->r, cd->g, cd->b, cd->a});
    return val;
}
VALUE rb_Shape_getOutlineColor(VALUE self) { return color_to_rb(get_shape(self)->shape.getOutlineColor()); }
VALUE rb_Shape_setOutlineColor(VALUE self, VALUE val)
{
    if (rb_obj_is_kind_of(val, rb_cColor) != Qtrue) return val;
    auto *cd = get_color_data(val);
    get_shape(self)->shape.setOutlineColor(cgss::Color{cd->r, cd->g, cd->b, cd->a});
    return val;
}
VALUE rb_Shape_getOutlineThickness(VALUE self) { return DBL2NUM(get_shape(self)->shape.getOutlineThickness()); }
VALUE rb_Shape_setOutlineThickness(VALUE self, VALUE val)
{
    get_shape(self)->shape.setOutlineThickness(static_cast<float>(NUM2DBL(val)));
    return val;
}

VALUE rb_Shape_getWidth(VALUE self) { return LONG2NUM(get_shape(self)->shape.getWidth()); }
VALUE rb_Shape_setWidth(VALUE self, VALUE val) { get_shape(self)->shape.setWidth(NUM2LONG(val)); return val; }
VALUE rb_Shape_getHeight(VALUE self) { return LONG2NUM(get_shape(self)->shape.getHeight()); }
VALUE rb_Shape_setHeight(VALUE self, VALUE val) { get_shape(self)->shape.setHeight(NUM2LONG(val)); return val; }

VALUE rb_Shape_getBitmap(VALUE self) { return get_shape(self)->rBitmap; }
VALUE rb_Shape_setBitmap(VALUE self, VALUE val)
{
    auto *s = get_shape(self);
    s->rBitmap = val;
    if (NIL_P(val)) {
        s->shape.setTexture(nullptr);
        return val;
    }
    if (rb_obj_is_kind_of(val, rb_cImage) != Qtrue) {
        rb_raise(rb_eRGSSError, "Shape#bitmap= expects a Bitmap (Image).");
    }
    auto *img = get_image(val);
    if (!img->valid()) {
        s->shape.setTexture(nullptr);
        return val;
    }
    s->shape.setTexture(&image_acquire_texture(img), true);
    return val;
}

VALUE rb_Shape_getBlendMode(VALUE self) { return get_shape(self)->rBlendMode; }
VALUE rb_Shape_setBlendMode(VALUE self, VALUE val)
{
    // Shape's BlendMode wiring follows the same path as ShaderSprite —
    // calls cgss::Shape::setRenderState. Stored Ruby-side for GC tracking.
    get_shape(self)->rBlendMode = val;
    return val;
}

// Backwards-compat: pre-Phase-4 the Ruby render loop drew Shape via
// explicit per-frame `shape.draw` calls. cgss::DisplayWindow::draw() now
// iterates the parent View's DrawableStack and renders the Shape
// automatically — kept as a no-op for callers that still issue it.
VALUE rb_Shape_draw(VALUE self)
{
    return self;
}

void Init_Shape()
{
    rb_cShape = rb_define_class_under(rb_mLiteRGSS, "Shape", rb_cDrawable);
    rb_define_alloc_func(rb_cShape, rb::Alloc<ShapeData>);

    rb_iShapeCircle = rb_intern("circle");
    rb_iShapeConvex = rb_intern("convex");
    rb_iShapeRectangle = rb_intern("rectangle");

    rb_define_method(rb_cShape, "initialize", _rbf rb_Shape_Initialize, -1);
    rb_define_method(rb_cShape, "dispose", _rbf rb_Shape_Dispose, 0);
    rb_define_method(rb_cShape, "disposed?", _rbf rb_Shape_Disposed, 0);
    rb_define_method(rb_cShape, "x", _rbf rb_Shape_getX, 0);
    rb_define_method(rb_cShape, "x=", _rbf rb_Shape_setX, 1);
    rb_define_method(rb_cShape, "y", _rbf rb_Shape_getY, 0);
    rb_define_method(rb_cShape, "y=", _rbf rb_Shape_setY, 1);
    rb_define_method(rb_cShape, "set_position", _rbf rb_Shape_setPosition, 2);
    rb_define_method(rb_cShape, "z", _rbf rb_Shape_getZ, 0);
    rb_define_method(rb_cShape, "z=", _rbf rb_Shape_setZ, 1);
    rb_define_method(rb_cShape, "ox", _rbf rb_Shape_getOX, 0);
    rb_define_method(rb_cShape, "ox=", _rbf rb_Shape_setOX, 1);
    rb_define_method(rb_cShape, "oy", _rbf rb_Shape_getOY, 0);
    rb_define_method(rb_cShape, "oy=", _rbf rb_Shape_setOY, 1);
    rb_define_method(rb_cShape, "set_origin", _rbf rb_Shape_setOrigin, 2);
    rb_define_method(rb_cShape, "angle", _rbf rb_Shape_getAngle, 0);
    rb_define_method(rb_cShape, "angle=", _rbf rb_Shape_setAngle, 1);
    rb_define_method(rb_cShape, "zoom_x", _rbf rb_Shape_getZoomX, 0);
    rb_define_method(rb_cShape, "zoom_x=", _rbf rb_Shape_setZoomX, 1);
    rb_define_method(rb_cShape, "zoom_y", _rbf rb_Shape_getZoomY, 0);
    rb_define_method(rb_cShape, "zoom_y=", _rbf rb_Shape_setZoomY, 1);
    rb_define_method(rb_cShape, "zoom=", _rbf rb_Shape_setZoom, 1);
    rb_define_method(rb_cShape, "viewport", _rbf rb_Shape_getViewport, 0);
    rb_define_method(rb_cShape, "visible", _rbf rb_Shape_getVisible, 0);
    rb_define_method(rb_cShape, "visible=", _rbf rb_Shape_setVisible, 1);
    rb_define_method(rb_cShape, "point_count", _rbf rb_Shape_getPointCount, 0);
    rb_define_method(rb_cShape, "point_count=", _rbf rb_Shape_setPointCount, 1);
    rb_define_method(rb_cShape, "get_point", _rbf rb_Shape_getPoint, 1);
    rb_define_method(rb_cShape, "set_point", _rbf rb_Shape_setPoint, 3);
    rb_define_method(rb_cShape, "color", _rbf rb_Shape_getColor, 0);
    rb_define_method(rb_cShape, "color=", _rbf rb_Shape_setColor, 1);
    rb_define_method(rb_cShape, "outline_color", _rbf rb_Shape_getOutlineColor, 0);
    rb_define_method(rb_cShape, "outline_color=", _rbf rb_Shape_setOutlineColor, 1);
    rb_define_method(rb_cShape, "outline_thickness", _rbf rb_Shape_getOutlineThickness, 0);
    rb_define_method(rb_cShape, "outline_thickness=", _rbf rb_Shape_setOutlineThickness, 1);
    rb_define_method(rb_cShape, "radius", _rbf rb_Shape_getRadius, 0);
    rb_define_method(rb_cShape, "radius=", _rbf rb_Shape_setRadius, 1);
    rb_define_method(rb_cShape, "type", _rbf rb_Shape_getType, 0);
    rb_define_method(rb_cShape, "width", _rbf rb_Shape_getWidth, 0);
    rb_define_method(rb_cShape, "width=", _rbf rb_Shape_setWidth, 1);
    rb_define_method(rb_cShape, "height", _rbf rb_Shape_getHeight, 0);
    rb_define_method(rb_cShape, "height=", _rbf rb_Shape_setHeight, 1);
    rb_define_method(rb_cShape, "bitmap", _rbf rb_Shape_getBitmap, 0);
    rb_define_method(rb_cShape, "bitmap=", _rbf rb_Shape_setBitmap, 1);
    rb_define_method(rb_cShape, "shader", _rbf rb_Shape_getBlendMode, 0);
    rb_define_method(rb_cShape, "shader=", _rbf rb_Shape_setBlendMode, 1);
    rb_define_method(rb_cShape, "blendmode", _rbf rb_Shape_getBlendMode, 0);
    rb_define_method(rb_cShape, "blendmode=", _rbf rb_Shape_setBlendMode, 1);
    rb_define_method(rb_cShape, "__index__", _rbf rb_Shape_Index, 0);
    rb_define_method(rb_cShape, "draw", _rbf rb_Shape_draw, 0);
    rb_define_method(rb_cShape, "clone", _rbf rb_Shape_Copy, 0);
    rb_define_method(rb_cShape, "dup", _rbf rb_Shape_Copy, 0);

    rb_define_const(rb_cShape, "CIRCLE", ID2SYM(rb_iShapeCircle));
    rb_define_const(rb_cShape, "CONVEX", ID2SYM(rb_iShapeConvex));
    rb_define_const(rb_cShape, "RECTANGLE", ID2SYM(rb_iShapeRectangle));
}
