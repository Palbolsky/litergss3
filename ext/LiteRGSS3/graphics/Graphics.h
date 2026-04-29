#ifndef Graphics_H
#define Graphics_H

#include "Color.h"
#include "DrawableDisposable.h"
#include "Rect.h"
#include "Tone.h"
#include "Table.h"
#include "Table32.h"
#include "BlendMode.h"
#include "Viewport.h"
#include "Sprite.h"
#include "Image.h"
#include "Text.h"
#include "Shape.h"
#include "SpriteMap.h"
#include "ShaderSprite.h"
#include "Shader.h"

inline void Init_Graphics()
{
    Init_DrawableDisposable();
    Init_Color();
    Init_Rect();
    Init_Tone();
    Init_Table();
    Init_Table32();
    Init_BlendMode();
    Init_Shader();
    Init_Image();
    Init_Viewport();
    Init_Sprite();
    Init_ShaderSprite();
    Init_Text();
    Init_Shape();
    Init_SpriteMap();
}

#endif