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
  // Ruby entry point. Name must match the compiled shared-library name
  // (gem_name="LiteRGSS3" → libLiteRGSS3.so → Init_LiteRGSS3). Step E of
  // the umbrella plan renamed both; the legacy top-level `LiteRGSS`
  // constant is kept as a Ruby-level alias for PSDK back-compat.
  void Init_LiteRGSS3()
  {
    rb_mLiteRGSS = rb_define_module("LiteRGSS3");

    // Back-compat alias — legacy scripts that reference `LiteRGSS::*` from
    // pre-step-E naming continue to resolve. Deprecated; remove in a
    // later major version once downstream consumers have migrated.
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
