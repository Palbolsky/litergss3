#ifndef Graphics_H
#define Graphics_H

#include "Color.h"
#include "Viewport.h"
#include "Sprite.h"
#include "Image.h"

inline void Init_Graphics()
{
    Init_Color();
    Init_Viewport();
    Init_Sprite();
    Init_Image();
}

#endif