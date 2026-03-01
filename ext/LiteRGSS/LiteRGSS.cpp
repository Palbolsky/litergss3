#include "LiteRGSS.h"
#include "window/Window.h"
#include "inputs/Inputs.h"

VALUE rb_mLiteRGSS = Qnil;
VALUE rb_mConfig   = Qnil;
VALUE rb_eRGSSError = Qnil;

extern "C"
{
  void Init_LiteRGSS()
  {
    rb_mLiteRGSS  = rb_define_module("LiteRGSS");
    rb_mConfig    = rb_define_module_under(rb_mLiteRGSS, "Config");
    rb_eRGSSError = rb_define_class_under(rb_mLiteRGSS, "Error", rb_eStandardError);

    Init_Window();
    Init_Inputs();
  }
}
