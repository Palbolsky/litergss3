#ifndef CGSS_PLATFORM_ANDROID_HOST_SURFACE_H
#define CGSS_PLATFORM_ANDROID_HOST_SURFACE_H

#if defined(__ANDROID__) || defined(ANDROID)
#include <jni.h>
#else
// Enough for the header to type-check off Android, where <jni.h> isn't
// available; the functions taking these types are no-ops there.
typedef struct _JavaVM JavaVM;
typedef void* jobject;
#endif

// Hosted-Activity bridge for Android. The Java side owns a SurfaceView; its
// Surface becomes an ANativeWindow* on the JNI side and is handed here, so
// the cgss::DisplayWindow renders into that surface instead of relying on a
// NativeActivity entry point. The host also pushes its input events here.
//
// Backend-neutral: the windowing part is implemented per backend
// (src/Backend/Specializations/<backend>/AndroidHostSurface.cpp). The SFML
// backend implements it (SFML-opengl-es2's hosted Android window); the
// raylib backend doesn't yet: there, attaching logs that it is unsupported.
//
// Include-safe from C++ and from C (a host's own JNI code): C consumers
// only see the C-linkage functions at the end. HostSurfaceJni.cpp is the
// JNI bridge a host registers on its class (cgss_android_register_natives).
//
// Every function is a no-op off Android (the render gate always passes).

#ifdef __cplusplus

namespace cgss::android {

	// ---- Surface lifecycle (UI thread) ----

	// Attach the host's ANativeWindow* (from ANativeWindow_fromSurface) as
	// the surface of the DisplayWindow. The caller keeps ownership of the
	// ANativeWindow* and releases it after detachHostSurface(). Attaching
	// again after a detach (back from the background) resumes rendering on
	// the new surface: the GL context survives in between.
	void attachHostSurface(void* nativeWindow, int width, int height);

	// The surface is going away (SurfaceHolder.Callback#surfaceDestroyed).
	// A pause, not a teardown: the render thread stops drawing (and waits,
	// see beginHostedFrame) while the GL context and the DisplayWindow stay
	// alive. Returns once no frame is in flight, as Android's contract asks.
	void detachHostSurface();

	void notifyHostSurfaceResized(int width, int height);

	// Push a Closed event to the DisplayWindow: its script gets on_closed
	// and stops. For the activity's onDestroy, before it blocks on the
	// interpreter's end: a render loop that never returns would deadlock it
	// otherwise. Also wakes a render loop paused in the background. Dropped
	// if no window exists yet: safe to call unconditionally.
	void requestHostSurfaceClose();

	// ---- Input pushed by the host ----

	void injectTouchDown(int pointerId, float x, float y);
	void injectTouchMove(int pointerId, float x, float y);
	void injectTouchUp(int pointerId, float x, float y);
	// Android KeyEvent key codes and meta state.
	void injectKeyDown(int androidKeyCode, int metaState, int repeatCount);
	void injectKeyUp(int androidKeyCode, int metaState);
	void injectText(unsigned int unicodeCodepoint);
	void injectFocusGained();
	void injectFocusLost();
	void injectJoystickButton(int deviceId, int androidKeyCode, bool pressed);
	void injectJoystickAxis(int   deviceId,
	                        float axisX,    float axisY,
	                        float axisZ,    float axisRz,
	                        float hatX,     float hatY,
	                        float lTrigger, float rTrigger);
	void injectJoystickConnected(int deviceId);
	void injectJoystickDisconnected(int deviceId);

	// ---- Virtual keyboard ----

	// Called with 1 (show) / 0 (hide) from the thread requesting it (the
	// render thread in practice): the host hops to its UI thread itself.
	typedef void (*VirtualKeyboardCallback)(int show);
	void setVirtualKeyboardCallback(VirtualKeyboardCallback cb);
	void requestVirtualKeyboard(bool show);

	// The host's JavaVM and Activity, for the backend's own JNI calls. Kept
	// across detach / attach (the Activity outlives its surfaces); a global
	// reference is taken on the Activity. (nullptr, nullptr) clears it.
	void setHostJniContext(JavaVM* vm, jobject activity);

	// ---- Window preservation (CGSS_WINDOW_PRESERVATION builds) ----

	// Opt-in, for every DisplayWindow constructed afterwards, to the
	// process-wide preserved window (otherwise only fullscreen settings
	// get it): it survives DisplayWindow lifetimes, GL context included,
	// so a game restarted in the same process doesn't rebuild everything.
	// Default false: each DisplayWindow owns its window, destroyed with it
	// (what a splash / loader wants, leaving a clean state behind).
	// The preserved window stays bound to the thread that created it:
	// keep constructing DisplayWindows on that thread.
	void setReuseSharedWindow(bool enabled);
	bool isReuseSharedWindowEnabled();

	// ---- Render thread gate (used by cgss::DisplayWindow) ----

	// Surround any GL work against the hosted surface outside a frame
	// (DisplayWindow::stop). True: go on, then call endHostedDraw. False:
	// detached, skip all GL work, don't call endHostedDraw.
	bool beginHostedDraw();
	void endHostedDraw();

	// beginHostedDraw for a frame. Blocks while the surface is detached
	// (the app in the background) until an attach or a close request: the
	// game pauses there. Call it outside any lock the UI thread needs and
	// outside the Ruby GVL, so other Ruby threads keep running.
	// `detaches` is the caller's count of the detaches it has seen (start
	// at 0). `surfaceChanged` tells whether the surface went away since the
	// caller's last frame: it then draws nothing until its event pump has
	// handled the LostFocus / GainedFocus pair (the backend rebuilds the
	// GL surface from them).
	// True: draw, then endHostedDraw. False: no lock held, skip the frame.
	bool beginHostedFrame(unsigned int& detaches, bool& surfaceChanged);

	// The event pump popped a Closed event: until then, a close request
	// keeps beginHostedFrame from waiting.
	void hostSurfaceCloseDelivered();

	// The event pump popped a LostFocus event. Until then, a detach's
	// LostFocus keeps beginHostedFrame from waiting: the script gets it
	// before the game pauses in the background (a host app about to be
	// killed there: the game saves on it), not once it is back.
	void hostSurfaceFocusLostDelivered();

	// The window closed or was recreated, its event queue with it, or its
	// DisplayWindow stopped using the preserved one: the Closed events queued
	// for it are no longer waited for, nor delivered to the next one (a close
	// request is addressed to the DisplayWindow live when it is made).
	// Likewise the LostFocus events not delivered yet.
	void hostSurfaceEventsDropped();

}

extern "C" {
#endif

	typedef void (*cgss_android_virtual_keyboard_callback_t)(int show);

	void cgss_android_attach_host_surface(void* nativeWindow, int width, int height);
	void cgss_android_detach_host_surface(void);
	void cgss_android_notify_host_surface_resized(int width, int height);
	void cgss_android_request_host_surface_close(void);
	void cgss_android_inject_touch_down(int pointerId, float x, float y);
	void cgss_android_inject_touch_move(int pointerId, float x, float y);
	void cgss_android_inject_touch_up(int pointerId, float x, float y);
	void cgss_android_inject_key_down(int androidKeyCode, int metaState, int repeatCount);
	void cgss_android_inject_key_up(int androidKeyCode, int metaState);
	void cgss_android_inject_text(unsigned int unicodeCodepoint);
	void cgss_android_inject_focus_gained(void);
	void cgss_android_inject_focus_lost(void);
	void cgss_android_inject_joystick_button(int deviceId, int androidKeyCode, int pressed);
	void cgss_android_inject_joystick_axis(int   deviceId,
	                                       float axisX,    float axisY,
	                                       float axisZ,    float axisRz,
	                                       float hatX,     float hatY,
	                                       float lTrigger, float rTrigger);
	void cgss_android_inject_joystick_connected(int deviceId);
	void cgss_android_inject_joystick_disconnected(int deviceId);
	void cgss_android_set_virtual_keyboard_callback(cgss_android_virtual_keyboard_callback_t cb);
	void cgss_android_request_virtual_keyboard(int show);
	void cgss_android_set_host_jni_context(JavaVM* vm, jobject activity);
	void cgss_android_set_reuse_shared_window(int enabled);
	int  cgss_android_is_reuse_shared_window_enabled(void);

	// Host surface listener: the host's own concerns (e.g. pausing its audio while
	// the app is in the background). Called with 1 once a surface is attached, 0
	// before it is detached; NULL clears it.
	typedef void (*cgss_android_host_surface_listener_t)(int attached);
	void cgss_android_set_host_surface_listener(cgss_android_host_surface_listener_t listener);

#if defined(__ANDROID__) || defined(ANDROID)
	// JNI bridge (HostSurfaceJni.cpp): registers the natives of the host's class
	// (attach, detach, inject*... see HostSurfaceJni.cpp for the list), which
	// declares them static, with `static void dispatchVirtualKeyboardRequest(
	// boolean)` for the IME callback. Typically from the host's JNI_OnLoad.
	// Returns JNI_OK, or a JNI error (an exception may be pending).
	int cgss_android_register_natives(JNIEnv* env, jclass hostClass);
#endif

#ifdef __cplusplus
}
#endif

#endif
