#ifndef Color_H
#define Color_H

#include "RubyValue.h"
#include "common/Normalize.h"

void Init_Color();
extern VALUE rb_cColor;

struct ColorData
{
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    bool operator==(const ColorData& other) const
    {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
};

#endif