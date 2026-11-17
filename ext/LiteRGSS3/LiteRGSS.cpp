#include "LiteRGSS.h"
#include "window/DisplayWindow.h"
#include "window/FramedView.h"
#include "yuki/Yuki.h"
#ifdef LITECGSS_USE_PHYSFS
#include "vfs/VfsLock.h"
#endif
#include "inputs/Inputs.h"
#include "graphics/Graphics.h"
#include "fonts/Fonts.h"
#include "events/Events.h"
#include "graphics/Image.h"
#include "agent/Agent.h"
#include <LiteCGSS/Common/Profiler.h>

#include <cstdlib>
#include <cstring>

VALUE rb_mLiteRGSS = Qnil;
VALUE rb_mConfig = Qnil;
VALUE rb_eRGSSError = Qnil;

#ifndef LITERGSS_INIT_FUNCTION
#error "LITERGSS_INIT_FUNCTION must name the extension's entry point (Init_<file basename>)"
#endif

extern "C"
{
  // Ruby entry point. Name must match the compiled shared-library basename
  // (LiteRGSS.so → Init_LiteRGSS, LiteRGSS_tracy.so → Init_LiteRGSS_tracy;
  // set by CMake as LITERGSS_INIT_FUNCTION). The internal Ruby
  // module is still named `LiteRGSS3` — `LiteRGSS` is aliased to it for
  // compat with callers that use the library's short name.
  static void stop_profiler(VALUE) { cgss::profiler::Stop(); }

  void LITERGSS_INIT_FUNCTION()
  {
    // A Tracy build (LiteRGSS_tracy) with no host to start the profiler
    // (tracy_ruby): LITERGSS_TRACY_START=1 starts it here, before anything
    // else of LiteCGSS runs (litergss-run does, for litergss-profile).
    const char* startTracy = std::getenv("LITERGSS_TRACY_START");
    if (cgss::profiler::Available() && startTracy != nullptr && std::strcmp(startTracy, "1") == 0) {
      cgss::profiler::Start();
      rb_set_end_proc(stop_profiler, Qnil);
    }

    rb_mLiteRGSS = rb_define_module("LiteRGSS3");

    // Short-name alias — PSDK and the in-repo test suite reference
    // `LiteRGSS::*` directly.
    rb_const_set(rb_cObject, rb_intern("LiteRGSS"), rb_mLiteRGSS);

    rb_mConfig = rb_define_module_under(rb_mLiteRGSS, "Config");
    rb_eRGSSError = rb_define_class_under(rb_mLiteRGSS, "Error", rb_eStandardError);

    // Event Struct classes must be registered before Window (which uses
    // them from its poll_event block yield).
    Init_Events();
    Init_DisplayWindow();
    Init_Inputs();
    Init_Graphics();
    // Bitmap alias — litergss2 had a separate Bitmap class (texture-side).
    // litergss3 consolidates: LiteRGSS::Bitmap == LiteRGSS::Image. PSDK
    // scripts that call `Bitmap.new(...)` continue to work unchanged.
    rb_const_set(rb_mLiteRGSS, rb_intern("Bitmap"), rb_cImage);
    Init_Fonts();
    // FramedView (LiteRGSS::Window — the UI widget) registers after
    // Graphics so it can inherit from rb_cDrawable.
    Init_FramedView();
    Init_Yuki();
    Init_YukiGifReader();
#ifdef LITECGSS_USE_PHYSFS
    Init_VfsLock();
#endif
    // Last: the runtime it may load refers to the classes above.
    Init_Agent();
  }
}
