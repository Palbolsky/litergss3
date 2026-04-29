#ifndef BlendMode_H
#define BlendMode_H

#include "RubyValue.h"
#include <LiteCGSS/Graphics/RenderStates.h>

extern VALUE rb_cBlendMode;
void Init_BlendMode();

// Ruby BlendMode wraps cgss::RenderStates — a BondElement that owns the
// blend factors/equations and (later, when Shader extends BlendMode) the
// shader binding. RenderStates::data() returns the editable backing.
using RenderStatesElement = cgss::RenderStates;

#endif
