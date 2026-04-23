#ifndef LiteRGSS_H
#define LiteRGSS_H

#include "RubyValue.h"

// Note: the <LiteCGSS2/Common/RaylibWrapper.h> include that used to live
// here was a transitive-propagation vehicle for the ext/graphics and
// ext/fonts TUs. Those TUs now #include it directly; this shared header
// stays backend-neutral so the inputs/* files (which route through
// LiteCGSS backend Ops and don't need raylib symbols) can include
// LiteRGSS.h cleanly. Step B removes the RaylibWrapper dependency entirely.

extern VALUE rb_mLiteRGSS;
extern VALUE rb_mConfig;
extern VALUE rb_eRGSSError;

#endif