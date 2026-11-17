#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wregister"

#include <ruby/ruby.h>  // the ruby.h shim only includes it: any Ruby header layout
#ifdef LITERGSS_INCLUDE_RUBY_THREAD
#include "ruby/thread.h"
#endif

#pragma GCC diagnostic pop

#define _rbf (VALUE(*)(...))