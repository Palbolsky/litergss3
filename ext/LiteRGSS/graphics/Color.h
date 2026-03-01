#ifndef Color_H
#define Color_H

#include "RubyValue.h"
#include "common/Normalize.h"

void Init_Color();
extern VALUE rb_cColor;
extern const rb_data_type_t color_type; // expose for other files

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

// Helper to extract ColorData from any Ruby Color value
inline ColorData* get_color_data(VALUE color)
{
    ColorData* cd;
    TypedData_Get_Struct(color, ColorData, &color_type, cd);
    return cd;
}

#endif