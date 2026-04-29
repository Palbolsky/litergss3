#ifndef Tone_H
#define Tone_H

#include "RubyValue.h"

extern VALUE rb_cTone;
void Init_Tone();

// 4-component float Tone (r, g, b in [-1.0, 1.0]; gray in [0.0, 1.0]).
// Public API exposes integer values in [-255, 255] / [0, 255] for compat
// with litergss2 PSDK scripts. Internal floats keep the resolution shaders
// consume directly when Tone reaches the GPU later (Shader uniform).
struct ToneData
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float gray = 0.0f;
};

ToneData *get_tone_data(VALUE self);

#endif
