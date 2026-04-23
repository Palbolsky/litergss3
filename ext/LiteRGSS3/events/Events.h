#ifndef Events_H
#define Events_H

#include "RubyValue.h"

#include "LiteCGSS/Events/Event.h"

// Ruby Struct classes for typed events yielded by Window#poll_event.
// Registered under the top-level LiteRGSS module; consumers dispatch via
// `case ev; when LiteRGSS::KeyEvent then ...; when LiteRGSS::ResizeEvent ...`
// or field access (`ev.code`, `ev.width`).
extern VALUE rb_cKeyEvent;
extern VALUE rb_cTextEvent;
extern VALUE rb_cMouseMoveEvent;
extern VALUE rb_cMouseButtonEvent;
extern VALUE rb_cMouseWheelEvent;
extern VALUE rb_cResizeEvent;
extern VALUE rb_cGamepadButtonEvent;
extern VALUE rb_cGamepadMoveEvent;
extern VALUE rb_cGamepadConnectEvent;
extern VALUE rb_cTouchEvent;
extern VALUE rb_cCloseEvent;
extern VALUE rb_cFocusEvent;

void Init_Events();

// Translate a cgss::Event into the matching Ruby Struct. Returns Qnil for
// event types not exposed to Ruby (e.g. SensorChanged, which neither
// backend currently produces).
VALUE build_ruby_event(const cgss::Event& ev);

#endif
