#ifndef Inputs_H
#define Inputs_H

#include "Mouse.h"
#include "Keyboard.h"
#include "Touch.h"
#include "Gamepad.h"

inline void Init_Inputs()
{
    Init_Mouse();
    Init_Keyboard();
    Init_Touch();
    Init_Gamepad();
}

#endif