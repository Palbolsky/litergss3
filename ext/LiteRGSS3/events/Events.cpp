#include "Events.h"
#include "LiteRGSS.h"

VALUE rb_cKeyEvent            = Qnil;
VALUE rb_cTextEvent           = Qnil;
VALUE rb_cMouseMoveEvent      = Qnil;
VALUE rb_cMouseButtonEvent    = Qnil;
VALUE rb_cMouseWheelEvent     = Qnil;
VALUE rb_cResizeEvent         = Qnil;
VALUE rb_cGamepadButtonEvent  = Qnil;
VALUE rb_cGamepadMoveEvent    = Qnil;
VALUE rb_cGamepadConnectEvent = Qnil;
VALUE rb_cTouchEvent          = Qnil;
VALUE rb_cCloseEvent          = Qnil;
VALUE rb_cFocusEvent          = Qnil;

void Init_Events()
{
	// Struct-of-N fields per event. Field names here are API-stable —
	// downstream scripts depend on `ev.code` / `ev.width` / etc. across
	// versions. See plan file risk note #2.
	// The first Struct member after the name is either a field name or a
	// block; the list is NULL-terminated.
	rb_cKeyEvent = rb_struct_define_under(rb_mLiteRGSS, "KeyEvent",
		"code", "alt", "control", "shift", "system", NULL);
	rb_cTextEvent = rb_struct_define_under(rb_mLiteRGSS, "TextEvent",
		"unicode", NULL);
	rb_cMouseMoveEvent = rb_struct_define_under(rb_mLiteRGSS, "MouseMoveEvent",
		"x", "y", NULL);
	rb_cMouseButtonEvent = rb_struct_define_under(rb_mLiteRGSS, "MouseButtonEvent",
		"button", "x", "y", NULL);
	rb_cMouseWheelEvent = rb_struct_define_under(rb_mLiteRGSS, "MouseWheelEvent",
		"delta", "x", "y", "horizontal", NULL);
	rb_cResizeEvent = rb_struct_define_under(rb_mLiteRGSS, "ResizeEvent",
		"width", "height", NULL);
	rb_cGamepadButtonEvent = rb_struct_define_under(rb_mLiteRGSS, "GamepadButtonEvent",
		"id", "button", NULL);
	rb_cGamepadMoveEvent = rb_struct_define_under(rb_mLiteRGSS, "GamepadMoveEvent",
		"id", "axis", "position", NULL);
	rb_cGamepadConnectEvent = rb_struct_define_under(rb_mLiteRGSS, "GamepadConnectEvent",
		"id", "connected", NULL);
	rb_cTouchEvent = rb_struct_define_under(rb_mLiteRGSS, "TouchEvent",
		"finger", "x", "y", "phase", NULL);
	// CloseEvent / FocusEvent carry no payload on SFML's Closed/LostFocus/
	// GainedFocus events beyond a bool — minimum viable Struct with one
	// field (FocusEvent) and zero fields (CloseEvent, still a Struct for
	// `case ev when LiteRGSS::CloseEvent` dispatch).
	rb_cCloseEvent = rb_struct_define_under(rb_mLiteRGSS, "CloseEvent", NULL);
	rb_cFocusEvent = rb_struct_define_under(rb_mLiteRGSS, "FocusEvent",
		"gained", NULL);
}

VALUE build_ruby_event(const cgss::Event& ev)
{
	switch (ev.type()) {
		case cgss::EventType::KeyPressed:
		case cgss::EventType::KeyReleased: {
			const auto payload = (ev.type() == cgss::EventType::KeyPressed)
				? ev.asKeyPress() : ev.asKeyRelease();
			if (!payload) return Qnil;
			return rb_struct_new(rb_cKeyEvent,
				INT2NUM(static_cast<int>(payload->code)),
				payload->alt     ? Qtrue : Qfalse,
				payload->control ? Qtrue : Qfalse,
				payload->shift   ? Qtrue : Qfalse,
				payload->system  ? Qtrue : Qfalse);
		}
		case cgss::EventType::TextEntered: {
			const auto payload = ev.asTextEntered();
			if (!payload) return Qnil;
			return rb_struct_new(rb_cTextEvent, UINT2NUM(payload->unicode));
		}
		case cgss::EventType::Resized: {
			const auto payload = ev.asResize();
			if (!payload) return Qnil;
			return rb_struct_new(rb_cResizeEvent,
				UINT2NUM(payload->width), UINT2NUM(payload->height));
		}
		case cgss::EventType::MouseMoved: {
			const auto payload = ev.asMouseMove();
			if (!payload) return Qnil;
			return rb_struct_new(rb_cMouseMoveEvent,
				INT2NUM(payload->x), INT2NUM(payload->y));
		}
		case cgss::EventType::MouseButtonPressed:
		case cgss::EventType::MouseButtonReleased: {
			const auto payload = (ev.type() == cgss::EventType::MouseButtonPressed)
				? ev.asMouseButtonPress() : ev.asMouseButtonRelease();
			if (!payload) return Qnil;
			return rb_struct_new(rb_cMouseButtonEvent,
				INT2NUM(static_cast<int>(payload->button)),
				INT2NUM(payload->x), INT2NUM(payload->y));
		}
		case cgss::EventType::MouseWheelScrolled: {
			const auto payload = ev.asMouseWheel();
			if (!payload) return Qnil;
			return rb_struct_new(rb_cMouseWheelEvent,
				DBL2NUM(static_cast<double>(payload->delta)),
				INT2NUM(payload->x), INT2NUM(payload->y),
				payload->horizontal ? Qtrue : Qfalse);
		}
		case cgss::EventType::JoystickButtonPressed:
		case cgss::EventType::JoystickButtonReleased: {
			const auto payload = (ev.type() == cgss::EventType::JoystickButtonPressed)
				? ev.asGamepadButtonPress() : ev.asGamepadButtonRelease();
			if (!payload) return Qnil;
			return rb_struct_new(rb_cGamepadButtonEvent,
				UINT2NUM(payload->id),
				INT2NUM(static_cast<int>(payload->button)));
		}
		case cgss::EventType::JoystickMoved: {
			const auto payload = ev.asGamepadMove();
			if (!payload) return Qnil;
			return rb_struct_new(rb_cGamepadMoveEvent,
				UINT2NUM(payload->id),
				INT2NUM(static_cast<int>(payload->axis)),
				DBL2NUM(static_cast<double>(payload->position)));
		}
		case cgss::EventType::JoystickConnected:
		case cgss::EventType::JoystickDisconnected: {
			const auto payload = (ev.type() == cgss::EventType::JoystickConnected)
				? ev.asGamepadConnect() : ev.asGamepadDisconnect();
			if (!payload) return Qnil;
			return rb_struct_new(rb_cGamepadConnectEvent,
				UINT2NUM(payload->id),
				(ev.type() == cgss::EventType::JoystickConnected) ? Qtrue : Qfalse);
		}
		case cgss::EventType::TouchBegan:
		case cgss::EventType::TouchMoved:
		case cgss::EventType::TouchEnded: {
			const auto payload = ev.asTouch();
			if (!payload) return Qnil;
			return rb_struct_new(rb_cTouchEvent,
				UINT2NUM(payload->finger),
				INT2NUM(payload->x), INT2NUM(payload->y),
				INT2NUM(static_cast<int>(payload->phase)));
		}
		case cgss::EventType::Closed:
			return rb_struct_new(rb_cCloseEvent);
		case cgss::EventType::GainedFocus:
			return rb_struct_new(rb_cFocusEvent, Qtrue);
		case cgss::EventType::LostFocus:
			return rb_struct_new(rb_cFocusEvent, Qfalse);
		case cgss::EventType::MouseEntered:
		case cgss::EventType::MouseLeft:
		case cgss::EventType::SensorChanged:
			// Not exposed to Ruby yet. Producer-side (SFML or raylib) may
			// or may not emit these; silently skip.
			return Qnil;
	}
	return Qnil;
}
