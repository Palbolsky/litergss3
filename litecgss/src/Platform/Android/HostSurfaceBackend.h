#ifndef CGSS_PLATFORM_ANDROID_HOST_SURFACE_BACKEND_H
#define CGSS_PLATFORM_ANDROID_HOST_SURFACE_BACKEND_H

// The windowing half of the Android host surface, implemented per backend
// in src/Backend/Specializations/<backend>/AndroidHostSurface.cpp. The
// shared half (Platform/Android/HostSurface.cpp) owns the lifecycle gate and
// calls these under its lock. Android builds only.

#if defined(__ANDROID__) || defined(ANDROID)

#include <jni.h>

namespace cgss::android::backend {

	// Bind the backend's (current or future) window to the host's native
	// window, creating the backend's hosted state on the first call.
	void attach(void* nativeWindow, int width, int height);
	// Forget the native window (released by the host right after).
	void detach();
	void resized(int width, int height);
	// Called after attach and whenever the JNI context changes.
	void setJniContext(JavaVM* vm, jobject activity);

	// Window events. Each returns true when the event reached a window's
	// queue (false: no window exists yet).
	bool pushFocus(bool gained);
	bool pushClosed();
	// Forget the close requests not delivered yet (hostSurfaceEventsDropped),
	// where they outlive their window: raylib keeps one window per process.
	void dropClosed();
	enum class TouchPhase { Began, Moved, Ended };
	bool pushTouch(TouchPhase phase, int pointerId, float x, float y);

	void pushKeyDown(int androidKeyCode, int metaState, int repeatCount);
	void pushKeyUp(int androidKeyCode, int metaState);
	void pushText(unsigned int unicodeCodepoint);
	void pushJoystickButton(int deviceId, int androidKeyCode, bool pressed);
	void pushJoystickAxis(int   deviceId,
	                      float axisX,    float axisY,
	                      float axisZ,    float axisRz,
	                      float hatX,     float hatY,
	                      float lTrigger, float rTrigger);
	void pushJoystickConnected(int deviceId);
	void pushJoystickDisconnected(int deviceId);

}

#endif
#endif
