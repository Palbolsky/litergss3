#include "LiteRGSS.h"
#include "window/Window.h"
#include "inputs/Inputs.h"
#include "graphics/Graphics.h"
#include "fonts/Fonts.h"
#include "events/Events.h"

VALUE rb_mLiteRGSS = Qnil;
VALUE rb_mConfig = Qnil;
VALUE rb_eRGSSError = Qnil;

extern "C"
{
  // Ruby entry point. Name must match the compiled shared-library basename
  // (ext_name="LiteRGSS" → LiteRGSS.so → Init_LiteRGSS). The internal Ruby
  // module is still named `LiteRGSS3` — `LiteRGSS` is aliased to it for
  // compat with callers that use the library's short name.
  void Init_LiteRGSS()
  {
    rb_mLiteRGSS = rb_define_module("LiteRGSS3");

    // Short-name alias — PSDK and the in-repo test suite reference
    // `LiteRGSS::*` directly.
    rb_const_set(rb_cObject, rb_intern("LiteRGSS"), rb_mLiteRGSS);

    rb_mConfig = rb_define_module_under(rb_mLiteRGSS, "Config");
    rb_eRGSSError = rb_define_class_under(rb_mLiteRGSS, "Error", rb_eStandardError);

    // Event Struct classes must be registered before Window (which uses
    // them from its poll_event block yield).
    Init_Events();
    Init_Window();
    Init_Inputs();
    Init_Graphics();
    Init_Fonts();
  }
}
