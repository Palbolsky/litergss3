#ifndef Image_H
#define Image_H

#include <LiteCGSS2/Common/RaylibWrapper.h>
#include "RubyValue.h"
#include "Color.h"
#include <LiteCGSS/Common/NormalizeNumbers.h>

void Init_Image();
extern VALUE rb_cImage;

struct ImageData
{
    raylib::Image image = {};
    bool disposed = false;

    int width() const { return image.width; }
    int height() const { return image.height; }
    bool valid() const { return image.data != nullptr; }
};
ImageData *get_image(VALUE self);

#endif