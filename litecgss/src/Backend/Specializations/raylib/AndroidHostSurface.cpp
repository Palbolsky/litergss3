// raylib half of the Android host surface (Platform/Android/HostSurfaceBackend.h):
// not supported yet. raylib's own Android platform owns a NativeActivity's
// window; rendering into a host's surface needs a raylib platform of ours
// (EGL on the given ANativeWindow, input fed by the host). Until then the
// host's calls are accepted and ignored, and attaching says so once.

#if defined(__ANDROID__) || defined(ANDROID)

#include <atomic>

#include <raylib.h>

#include "Platform/Android/HostSurfaceBackend.h"

namespace cgss::android::backend {

	void attach(void*, int, int) {
		static std::atomic<bool> warned { false };
		if (!warned.exchange(true)) {
			TraceLog(LOG_WARNING, "LiteCGSS: the Android host surface is not supported by the raylib backend yet");
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
