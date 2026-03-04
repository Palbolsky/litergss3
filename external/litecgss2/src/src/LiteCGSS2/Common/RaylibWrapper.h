#ifndef Raylib_Wrapper_H
#define Raylib_Wrapper_H

#include <cstdarg>

#ifdef LoadImage
#undef LoadImage
#endif
#ifdef CloseWindow
#undef CloseWindow
#endif
#ifdef ShowCursor
#undef ShowCursor
#endif

namespace raylib
{
#include "raylib.h"
}

inline raylib::Image raylib_LoadImage(const char* path) { return raylib::LoadImage(path); }
inline void raylib_UnloadImage(raylib::Image image)     { raylib::UnloadImage(image); }

#endif