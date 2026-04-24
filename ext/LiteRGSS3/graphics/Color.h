#ifndef Color_H
#define Color_H

#include "RubyValue.h"
#include "../rbAdapter.h"
#include <LiteCGSS/Common/NormalizeNumbers.h>

void Init_Color();
extern VALUE rb_cColor;

struct ColorData
{
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    bool operator==(const ColorData &other) const
    {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
};

inline ColorData *get_color_data(VALUE color) { return rb::GetPtr<ColorData>(color); }

#endif