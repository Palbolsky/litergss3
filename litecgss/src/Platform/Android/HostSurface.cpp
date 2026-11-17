// Android host surface: the lifecycle gate between the host's UI thread
// (attach / detach / close) and the render thread (frames, event pump),
// plus the host-side state (JNI context, virtual keyboard callback, window
// preservation opt-in). The windowing calls go to the active backend
// (Platform/Android/HostSurfaceBackend.h). Port of LiteCGSS2's HostSurface.

#include "LiteCGSS/Platform/Android/HostSurface.h"

#include <atomic>

namespace {
	// Read by the render thread (DisplayWindow's constructor), written by
	// the UI thread.
	std::atomic<bool> g_reuseSharedWindow { false };

	// Called from whichever thread requests the keyboard; a single pointer.
	std::atomic<cgss::android::VirtualKeyboardCallback> g_virtualKeyboardCallback { nullptr };
}

namespace cgss::android {

	void setReuseSharedWindow(bool enabled) {
		g_reuseSharedWindow.store(enabled, std::memory_order_release);
	}

	bool isReuseSharedWindowEnabled() {
		return g_reuseSharedWindow.load(std::memory_order_acquire);
	}

	void setVirtualKeyboardCallback(VirtualKeyboardCallback cb) {
		g_virtualKeyboardCallback.store(cb, std::memory_order_release);
	}

	void requestVirtualKeyboard(bool show) {
		if (const auto cb = g_virtualKeyboardCallback.load(std::memory_order_acquire)) {
			cb(show ? 1 : 0);
		}
	}

}

#if defined(__ANDROID__) || defined(ANDROID)

#include <condition_variable>
#include <mutex>

#include "Platform/Android/HostSurfaceBackend.h"

namespace {

	// Serializes the UI thread (attach / detach / close) against the render
	// thread's frames: one hosted surface, one render thread per process.
	std::mutex g_mutex;

	// Read without the lock to short-cut; always written under it.
	std::atomic<bool> g_attached { false };

	// Wakes beginHostedFrame: an attach or a close request. Under g_mutex.
	std::condition_variable g_changed;

	// Closed events queued and not popped yet (hostSurfaceCloseDelivered)
	// nor lost with their window (hostSurfaceEventsDropped): while there
	// are, frames don't wait, as a loop not pumping events (update_no_input)
	// would never get them. Under g_mutex.
	unsigned int g_closesQueued = 0;

	// Detaches so far: a frame finding the surface back still knows it went
	// away since its last one. Under g_mutex.
	unsigned int g_detaches = 0;

	// The host's JavaVM + a global reference on its Activity. Outlives
	// detach / attach cycles. Under g_mutex.
	JavaVM* g_jvm = nullptr;
	jobject g_activity = nullptr;

	// A JNIEnv for the calling thread, attaching it if needed
	// (*attached: the caller must DetachCurrentThread).
	JNIEnv* acquireEnv(JavaVM* vm, bool* attached) {
		*attached = false;
		if (vm == nullptr) return nullptr;
		JNIEnv* env = nullptr;
		const jint rc = vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
		if (rc == JNI_OK) return env;
		if (rc == JNI_EDETACHED) {
			JavaVMAttachArgs args;
			args.version = JNI_VERSION_1_6;
			args.name    = const_cast<char*>("LiteCGSS-JniCtx");
			args.group   = nullptr;
			if (vm->AttachCurrentThread(&env, &args) == JNI_OK) {
				*attached = true;
				return env;
			}
		}
		return nullptr;
	}

	// Under g_mutex.
	void releaseActivity() {
		if (g_activity != nullptr && g_jvm != nullptr) {
			bool attached = false;
			if (JNIEnv* env = acquireEnv(g_jvm, &attached)) {
				env->DeleteGlobalRef(g_activity);
			}
			if (attached) g_jvm->DetachCurrentThread();
		}
		g_activity = nullptr;
	}

}

namespace cgss::android {

	void attachHostSurface(void* nativeWindow, int width, int height) {
		std::lock_guard<std::mutex> lock(g_mutex);
		// The backend's hosted state (GL display and context) is kept across
		// detaches: attaching again only rebinds the native window.
		backend::attach(nativeWindow, width, height);
		if (g_jvm != nullptr && g_activity != nullptr) {
			backend::setJniContext(g_jvm, g_activity);
		}
		g_attached.store(true, std::memory_order_release);
		g_changed.notify_all();
		// The window rebuilds its GL surface on the new native window when
		// its event pump handles this (no window yet: its creation does).
		backend::pushFocus(true);
	}

	void detachHostSurface() {
		std::lock_guard<std::mutex> lock(g_mutex);
		// A pause, not a teardown: frames stop (and wait), the window drops
		// its GL surface when its pump handles the LostFocus, the context and
		// the DisplayWindow live on. The script's own end (the host stopping
		// the interpreter) destroys the window.
		g_attached.store(false, std::memory_order_release);
		++g_detaches;
		backend::pushFocus(false);
		backend::detach();
	}

	void notifyHostSurfaceResized(int width, int height) {
		backend::resized(width, height);
	}

	bool beginHostedDraw() {
		g_mutex.lock();
		if (!g_attached.load(std::memory_order_acquire)) {
			g_mutex.unlock();
			return false;
		}
		return true;
	}

	void endHostedDraw() {
		g_mutex.unlock();
	}

	bool beginHostedFrame(unsigned int& detaches, bool& surfaceChanged) {
		std::unique_lock<std::mutex> lock(g_mutex);
		g_changed.wait(lock, [] {
			return g_attached.load(std::memory_order_acquire) || g_closesQueued > 0;
		});
		surfaceChanged = detaches != g_detaches;
		detaches = g_detaches;
		if (surfaceChanged || !g_attached.load(std::memory_order_acquire)) {
			return false;
		}
		// Held until endHostedDraw: no detach before the frame is done
		lock.release();
		return true;
	}

	void hostSurfaceCloseDelivered() {
		std::lock_guard<std::mutex> lock(g_mutex);
		if (g_closesQueued > 0) --g_closesQueued;
	}

	void hostSurfaceEventsDropped() {
		std::lock_guard<std::mutex> lock(g_mutex);
		g_closesQueued = 0;
		backend::dropClosed();
	}

	void requestHostSurfaceClose() {
		{
			// Queued before a waiting render thread can wake: its pump must
			// find the Closed event, or it would wait again for good.
			std::lock_guard<std::mutex> lock(g_mutex);
			if (backend::pushClosed()) {
				++g_closesQueued;
			}
		}
		g_changed.notify_all();
	}

	void injectTouchDown(int pointerId, float x, float y) {
		backend::pushTouch(backend::TouchPhase::Began, pointerId, x, y);
	}
	void injectTouchMove(int pointerId, float x, float y) {
		backend::pushTouch(backend::TouchPhase::Moved, pointerId, x, y);
	}
	void injectTouchUp(int pointerId, float x, float y) {
		backend::pushTouch(backend::TouchPhase::Ended, pointerId, x, y);
	}
	void injectKeyDown(int androidKeyCode, int metaState, int repeatCount) {
		backend::pushKeyDown(androidKeyCode, metaState, repeatCount);
	}
	void injectKeyUp(int androidKeyCode, int metaState) {
		backend::pushKeyUp(androidKeyCode, metaState);
	}
	void injectText(unsigned int unicodeCodepoint) {
		backend::pushText(unicodeCodepoint);
	}
	void injectFocusGained() {
		backend::pushFocus(true);
	}
	void injectFocusLost() {
		backend::pushFocus(false);
	}
	void injectJoystickButton(int deviceId, int androidKeyCode, bool pressed) {
		backend::pushJoystickButton(deviceId, androidKeyCode, pressed);
	}
	void injectJoystickAxis(int   deviceId,
	                        float axisX,    float axisY,
	                        float axisZ,    float axisRz,
	                        float hatX,     float hatY,
	                        float lTrigger, float rTrigger) {
		backend::pushJoystickAxis(deviceId, axisX, axisY, axisZ, axisRz, hatX, hatY, lTrigger, rTrigger);
	}
	void injectJoystickConnected(int deviceId) {
		backend::pushJoystickConnected(deviceId);
	}
	void injectJoystickDisconnected(int deviceId) {
		backend::pushJoystickDisconnected(deviceId);
	}

	void setHostJniContext(JavaVM* vm, jobject activity) {
		std::lock_guard<std::mutex> lock(g_mutex);
		// Replaces the previous one (Activity recreated on a configuration
		// change) or clears it (nullptr, nullptr).
		releaseActivity();
		g_jvm = vm;
		if (vm != nullptr && activity != nullptr) {
			bool attached = false;
			if (JNIEnv* env = acquireEnv(vm, &attached)) {
				// Accepts local or global references, always returns a global one.
				g_activity = env->NewGlobalRef(activity);
			}
			if (attached) vm->DetachCurrentThread();
		}
		// Not attached: the next attach forwards it.
		if (g_attached.load(std::memory_order_acquire)) {
			backend::setJniContext(g_jvm, g_activity);
		}
	}

}

#else

// Off Android: nothing hosts a surface, the gate always passes.
namespace cgss::android {
	void attachHostSurface(void*, int, int) {}
	void detachHostSurface() {}
	void notifyHostSurfaceResized(int, int) {}
	void requestHostSurfaceClose() {}
	void injectTouchDown(int, float, float) {}
	void injectTouchMove(int, float, float) {}
	void injectTouchUp(int, float, float) {}
	void injectKeyDown(int, int, int) {}
	void injectKeyUp(int, int) {}
	void injectText(unsigned int) {}
	void injectFocusGained() {}
	void injectFocusLost() {}
	void injectJoystickButton(int, int, bool) {}
	void injectJoystickAxis(int, float, float, float, float, float, float, float, float) {}
	void injectJoystickConnected(int) {}
	void injectJoystickDisconnected(int) {}
	void setHostJniContext(JavaVM*, jobject) {}
	bool beginHostedDraw() { return true; }
	void endHostedDraw() {}
	bool beginHostedFrame(unsigned int&, bool& surfaceChanged) {
		surfaceChanged = false;
		return true;
	}
	void hostSurfaceCloseDelivered() {}
	void hostSurfaceEventsDropped() {}
}

#endif

// C entry points for the JNI bridge: same on every platform.
extern "C" {

	void cgss_android_attach_host_surface(void* nativeWindow, int width, int height) {
		cgss::android::attachHostSurface(nativeWindow, width, height);
	}
	void cgss_android_detach_host_surface(void) {
		cgss::android::detachHostSurface();
	}
	void cgss_android_notify_host_surface_resized(int width, int height) {
		cgss::android::notifyHostSurfaceResized(width, height);
	}
	void cgss_android_request_host_surface_close(void) {
		cgss::android::requestHostSurfaceClose();
	}
	void cgss_android_inject_touch_down(int pointerId, float x, float y) {
		cgss::android::injectTouchDown(pointerId, x, y);
	}
	void cgss_android_inject_touch_move(int pointerId, float x, float y) {
		cgss::android::injectTouchMove(pointerId, x, y);
	}
	void cgss_android_inject_touch_up(int pointerId, float x, float y) {
		cgss::android::injectTouchUp(pointerId, x, y);
	}
	void cgss_android_inject_key_down(int androidKeyCode, int metaState, int repeatCount) {
		cgss::android::injectKeyDown(androidKeyCode, metaState, repeatCount);
	}
	void cgss_android_inject_key_up(int androidKeyCode, int metaState) {
		cgss::android::injectKeyUp(androidKeyCode, metaState);
	}
	void cgss_android_inject_text(unsigned int unicodeCodepoint) {
		cgss::android::injectText(unicodeCodepoint);
	}
	void cgss_android_inject_focus_gained(void) {
		cgss::android::injectFocusGained();
	}
	void cgss_android_inject_focus_lost(void) {
		cgss::android::injectFocusLost();
	}
	void cgss_android_inject_joystick_button(int deviceId, int androidKeyCode, int pressed) {
		cgss::android::injectJoystickButton(deviceId, androidKeyCode, pressed != 0);
	}
	void cgss_android_inject_joystick_axis(int   deviceId,
	                                       float axisX,    float axisY,
	                                       float axisZ,    float axisRz,
	                                       float hatX,     float hatY,
	                                       float lTrigger, float rTrigger) {
		cgss::android::injectJoystickAxis(deviceId, axisX, axisY, axisZ, axisRz, hatX, hatY, lTrigger, rTrigger);
	}
	void cgss_android_inject_joystick_connected(int deviceId) {
		cgss::android::injectJoystickConnected(deviceId);
	}
	void cgss_android_inject_joystick_disconnected(int deviceId) {
		cgss::android::injectJoystickDisconnected(deviceId);
	}
	void cgss_android_set_virtual_keyboard_callback(cgss_android_virtual_keyboard_callback_t cb) {
		cgss::android::setVirtualKeyboardCallback(cb);
	}
	void cgss_android_request_virtual_keyboard(int show) {
		cgss::android::requestVirtualKeyboard(show != 0);
	}
	void cgss_android_set_host_jni_context(JavaVM* vm, jobject activity) {
		cgss::android::setHostJniContext(vm, activity);
	}
	void cgss_android_set_reuse_shared_window(int enabled) {
		cgss::android::setReuseSharedWindow(enabled != 0);
	}
	int cgss_android_is_reuse_shared_window_enabled(void) {
		return cgss::android::isReuseSharedWindowEnabled() ? 1 : 0;
	}

}
