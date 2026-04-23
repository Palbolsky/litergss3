#ifndef LiteRGSS_H
#define LiteRGSS_H

#include "RubyValue.h"

// Shared top-level header. Intentionally backend-neutral: the inputs/ +
// events/ + window/ layers route through cgss backend Ops and don't need
// raylib symbols; the raylib-coupled graphics/ + fonts/ TUs include
// <raylib.h> directly in their own headers.

extern VALUE rb_mLiteRGSS;
extern VALUE rb_mConfig;
extern VALUE rb_eRGSSError;

#endif