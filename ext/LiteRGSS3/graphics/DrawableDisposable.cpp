#include "LiteRGSS.h"
#include "DrawableDisposable.h"

VALUE rb_cDisposable = Qnil;
VALUE rb_cDrawable = Qnil;

// Stub base methods. Subclasses override these via their own
// rb_define_method to plug in per-class disposed flags / cleanup.
static VALUE rb_DrawableDisposable_dispose(VALUE self) { return self; }

static VALUE rb_DrawableDisposable_disposed(VALUE self)
{
    rb_check_type(self, T_DATA);
    return (RDATA(self)->data == nullptr ? Qtrue : Qfalse);
}

void Init_DrawableDisposable()
{
    rb_cDisposable = rb_define_class_under(rb_mLiteRGSS, "Disposable", rb_cObject);
    rb_define_method(rb_cDisposable, "dispose", _rbf rb_DrawableDisposable_dispose, 0);
    rb_define_method(rb_cDisposable, "disposed?", _rbf rb_DrawableDisposable_disposed, 0);

    rb_cDrawable = rb_define_class_under(rb_mLiteRGSS, "Drawable", rb_cDisposable);
}
