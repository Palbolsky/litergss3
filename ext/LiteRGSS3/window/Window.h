#ifndef Window_H
#define Window_H

#include "RubyValue.h"

extern VALUE rb_cWindow;
void Init_Window();

extern int base_width;
extern int base_height;
extern double window_scale;

#endif