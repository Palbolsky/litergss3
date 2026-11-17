// raylib half of the Android host surface (Platform/Android/HostSurfaceBackend.h).
//
// raylib's own Android platform owns a NativeActivity; rendering into the
// host's surface needs raylib built PLATFORM=AndroidHosted (a raylib patch,
// litergss-everywhere's cmake/litergss-app/patches/raylib/android), whose
// host API is raylib_android_hosted.h. With it, the host's surface and input
// go to raylib (recorded there, applied by the render thread at its next
// frame or event pump: RaylibBackend's window_poll_event). Without it, the
// host's calls are accepted and ignored, and attaching says so once.

#if defined(__ANDROID__) || defined(ANDROID)

#include "Platform/Android/HostSurfaceBackend.h"

#if __has_include(<raylib_android_hosted.h>)

#include <raylib_android_hosted.h>

namespace cgss::android::backend {

	void attach(void* nativeWindow, int width, int height) {
		// Kept across detaches by raylib: the EGL context and everything in
		// it survive, the render thread only binds a new window surface.
		RaylibHostedSetWindow(nativeWindow, width, height);
	}

	void detach() {
		RaylibHostedClearWindow();
	}

	void resized(int width, int height) {
		RaylibHostedResize(width, height);
	}

	// raylib needs no JNI context (the host opens URLs, owns the clipboard)
	void setJniContext(JavaVM*, jobject) {}

	// Events reach a window once raylib's InitWindow is done
	bool pushFocus(bool gained) {
		if (!RaylibHostedIsReady()) return false;
		RaylibHostedPushFocus(gained);
		return true;
	}

	bool pushClosed() {
		if (!RaylibHostedIsReady()) return false;
		RaylibHostedPushClose();
		return true;
	}

	bool pushTouch(TouchPhase phase, int pointerId, float x, float y) {
		if (!RaylibHostedIsReady()) return false;
		RaylibHostedPushTouch(phase == TouchPhase::Began ? RAYLIB_HOSTED_TOUCH_DOWN
		                    : phase == TouchPhase::Moved ? RAYLIB_HOSTED_TOUCH_MOVE
		                    : RAYLIB_HOSTED_TOUCH_UP, pointerId, x, y);
		return true;
	}

	void pushKeyDown(int androidKeyCode, int /*metaState*/, int /*repeatCount*/) {
		RaylibHostedPushKey(androidKeyCode, true);
	}

	void pushKeyUp(int androidKeyCode, int /*metaState*/) {
		RaylibHostedPushKey(androidKeyCode, false);
	}

	void pushText(unsigned int unicodeCodepoint) {
		RaylibHostedPushText(unicodeCodepoint);
	}

	void pushJoystickButton(int deviceId, int androidKeyCode, bool pressed) {
		RaylibHostedPushGamepadButton(deviceId, androidKeyCode, pressed);
	}

	void pushJoystickAxis(int   deviceId,
	                      float axisX,    float axisY,
	                      float axisZ,    float axisRz,
	                      float hatX,     float hatY,
	                      float lTrigger, float rTrigger) {
		RaylibHostedPushGamepadAxes(deviceId, axisX, axisY, axisZ, axisRz, hatX, hatY, lTrigger, rTrigger);
	}

	void pushJoystickConnected(int deviceId) {
		RaylibHostedPushGamepadConnected(deviceId, true);
	}

	void pushJoystickDisconnected(int deviceId) {
		RaylibHostedPushGamepadConnected(deviceId, false);
	}

}

#else // a raylib without PLATFORM=AndroidHosted

#include <atomic>

#include <raylib.h>

namespace cgss::android::backend {

	void attach(void*, int, int) {
		static std::atomic<bool> warned { false };
		if (!warned.exchange(true)) {
			TraceLog(LOG_WARNING, "LiteCGSS: this raylib has no hosted Android platform (PLATFORM=AndroidHosted): "
			                      "the host's surface is ignored");
		}
	}

	void detach() {}
	void resized(int, int) {}
	void setJniContext(JavaVM*, jobject) {}
	bool pushFocus(bool) { return false; }
	bool pushClosed() { return false; }
	bool pushTouch(TouchPhase, int, float, float) { return false; }
	void pushKeyDown(int, int, int) {}
	void pushKeyUp(int, int) {}
	void pushText(unsigned int) {}
	void pushJoystickButton(int, int, bool) {}
	void pushJoystickAxis(int, float, float, float, float, float, float, float, float) {}
	void pushJoystickConnected(int) {}
	void pushJoystickDisconnected(int) {}

}

#endif

#endif
