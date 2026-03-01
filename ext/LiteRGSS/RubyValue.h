#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"

#include "ruby.h"
#ifdef LITERGSS_INCLUDE_RUBY_THREAD
#include "ruby/thread.h"
#endif

#pragma GCC diagnostic pop

#define _rbf (VALUE(*)(...))