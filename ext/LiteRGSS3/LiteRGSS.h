#ifndef LiteRGSS_H
#define LiteRGSS_H

#include "RubyValue.h"

// Shared top-level header. Backend-neutral, like every LiteRGSS TU: the
// bindings only use LiteCGSS' public API (litecgss/include/), never a
// graphics backend nor the engine's private Ops.

extern VALUE rb_mLiteRGSS;
extern VALUE rb_mConfig;
extern VALUE rb_eRGSSError;

#endif