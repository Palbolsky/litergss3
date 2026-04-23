#ifndef Image_H
#define Image_H

#include "RubyValue.h"
#include "Color.h"
#include <LiteCGSS/Common/NormalizeNumbers.h>
#include <LiteCGSS/Image/Image.h>

void Init_Image();
extern VALUE rb_cImage;

// Ruby wrapper around cgss::Image. Backend-agnostic: loads/saves/edits
// route through cgss::Image + backend Ops, so the same file compiles
// under CGSS_BACKEND=sfml and raylib (provided the raylib-side
// ImageSerializer specializations are present, which they now are —
// see Backend/Specializations/raylib/ImageSerializer.cpp).
struct ImageData
{
    cgss::Image image;
    bool disposed = false;

    unsigned int width() const { return image.width(); }
    unsigned int height() const { return image.height(); }
    bool valid() const { return image.width() > 0 && image.height() > 0; }
};
ImageData *get_image(VALUE self);

#endif
