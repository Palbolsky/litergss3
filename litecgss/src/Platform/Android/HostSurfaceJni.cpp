// JNI bridge of the Android host surface: the natives of the host's Java /
// Kotlin class (a SurfaceView's Surface, its touch / key / gamepad events,
// the IME request back to the host) forwarded to the cgss_android_* API.
//
// The host owns the class and its name: cgss_android_register_natives binds
// these natives to it (RegisterNatives, typically from the host's
// JNI_OnLoad). The class declares them static (Kotlin: an object with
// @JvmStatic external funs), with the names and signatures of kNatives
// below, plus `static void dispatchVirtualKeyboardRequest(boolean)`, which
// the IME callback calls.
//
// Threading: attach / detach / resized come from the UI thread
// (SurfaceHolder.Callback). They only hand the ANativeWindow over and need
// no GL context: the render thread binds the EGL surface when the
// DisplayWindow is created, and owns the context from then on. The input
// injections are safe from the UI thread too (the event queue is guarded).

#include "LiteCGSS/Platform/Android/HostSurface.h"

#if defined(__ANDROID__) || defined(ANDROID)

#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <atomic>


namespace {

	// The single live ANativeWindow*, like the single host surface: acquired
	// by ANativeWindow_fromSurface, released on detach / re-attach.
	ANativeWindow* g_window = nullptr;

	std::atomic<cgss_android_host_surface_listener_t> g_listener{nullptr};

	// IME: the thunk calls the host class back, from any thread
	JavaVM* g_jvm = nullptr;
	std::atomic<jclass> g_hostClass{nullptr}; // global ref
	jmethodID g_dispatchVirtualKeyboard = nullptr;

	void notifyListener(int attached) {
		cgss_android_host_surface_listener_t listener = g_listener.load();
		if (listener != nullptr) {
			listener(attached);
		}
	}

	void JNICALL attach(JNIEnv* env, jclass, jobject surface, jint width, jint height) {
		if (surface == nullptr) {
			return;
		}
		ANativeWindow* nextWindow = ANativeWindow_fromSurface(env, surface);
		if (nextWindow == nullptr) {
			return;
		}
		// A Surface still held (a new one without a detach, e.g. an
		// orientation change): release it first, attaching twice in a row
		// replaces the window.
		if (g_window != nullptr) {
			ANativeWindow_release(g_window);
		}
		g_window = nextWindow;
		cgss_android_attach_host_surface(g_window, width, height);
		notifyListener(1);
	}

	void JNICALL resized(JNIEnv*, jclass, jint width, jint height) {
		cgss_android_notify_host_surface_resized(width, height);
	}

	void JNICALL detach(JNIEnv*, jclass) {
		notifyListener(0);
		cgss_android_detach_host_surface();
		if (g_window != nullptr) {
			ANativeWindow_release(g_window);
			g_window = nullptr;
		}
	}

	void JNICALL requestClose(JNIEnv*, jclass) {
		cgss_android_request_host_surface_close();
	}

	void JNICALL injectTouchDown(JNIEnv*, jclass, jint pointerId, jfloat x, jfloat y) {
		cgss_android_inject_touch_down(pointerId, x, y);
	}

	void JNICALL injectTouchMove(JNIEnv*, jclass, jint pointerId, jfloat x, jfloat y) {
		cgss_android_inject_touch_move(pointerId, x, y);
	}

	void JNICALL injectTouchUp(JNIEnv*, jclass, jint pointerId, jfloat x, jfloat y) {
		cgss_android_inject_touch_up(pointerId, x, y);
	}

	void JNICALL injectKeyDown(JNIEnv*, jclass, jint androidKeyCode, jint metaState, jint repeatCount) {
		cgss_android_inject_key_down(androidKeyCode, metaState, repeatCount);
	}

	void JNICALL injectKeyUp(JNIEnv*, jclass, jint androidKeyCode, jint metaState) {
		cgss_android_inject_key_up(androidKeyCode, metaState);
	}

	void JNICALL injectText(JNIEnv*, jclass, jint unicodeCodepoint) {
		cgss_android_inject_text(static_cast<unsigned int>(unicodeCodepoint));
	}

	void JNICALL injectFocusGained(JNIEnv*, jclass) {
		cgss_android_inject_focus_gained();
	}

	void JNICALL injectFocusLost(JNIEnv*, jclass) {
		cgss_android_inject_focus_lost();
	}

	void JNICALL injectJoystickButton(JNIEnv*, jclass, jint deviceId, jint androidKeyCode, jboolean pressed) {
		cgss_android_inject_joystick_button(deviceId, androidKeyCode, pressed == JNI_TRUE ? 1 : 0);
	}

	void JNICALL injectJoystickAxis(JNIEnv*, jclass, jint deviceId,
	                                jfloat axisX, jfloat axisY, jfloat axisZ, jfloat axisRz,
	                                jfloat hatX, jfloat hatY, jfloat lTrigger, jfloat rTrigger) {
		cgss_android_inject_joystick_axis(deviceId, axisX, axisY, axisZ, axisRz,
		                                  hatX, hatY, lTrigger, rTrigger);
	}

	void JNICALL injectJoystickConnected(JNIEnv*, jclass, jint deviceId) {
		cgss_android_inject_joystick_connected(deviceId);
	}

	void JNICALL injectJoystickDisconnected(JNIEnv*, jclass, jint deviceId) {
		cgss_android_inject_joystick_disconnected(deviceId);
	}

	// Called from whichever thread requests the keyboard (typically the
	// render thread): attaches it to the JVM if needed. Listeners on the
	// host side hop to the UI thread themselves.
	void virtualKeyboardThunk(int show) {
		jclass hostClass = g_hostClass.load(std::memory_order_acquire);
		if (g_jvm == nullptr || hostClass == nullptr) {
			return;
		}
		JNIEnv* env = nullptr;
		bool attached = false;
		jint rc = g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
		if (rc == JNI_EDETACHED) {
			if (g_jvm->AttachCurrentThread(&env, nullptr) != JNI_OK) {
				return;
			}
			attached = true;
		} else if (rc != JNI_OK || env == nullptr) {
			return;
		}

		env->CallStaticVoidMethod(hostClass, g_dispatchVirtualKeyboard, show ? JNI_TRUE : JNI_FALSE);
		// A listener's exception must not cross into native code
		if (env->ExceptionCheck()) {
			env->ExceptionDescribe();
			env->ExceptionClear();
		}

		if (attached) {
			g_jvm->DetachCurrentThread();
		}
	}

	void JNICALL nativeSetVirtualKeyboardCallback(JNIEnv*, jclass, jboolean armed) {
		cgss_android_virtual_keyboard_callback_t callback =
			armed == JNI_TRUE && g_hostClass.load() != nullptr ? virtualKeyboardThunk : nullptr;
		cgss_android_set_virtual_keyboard_callback(callback);
	}

	void JNICALL nativeSetHostActivity(JNIEnv*, jclass, jobject activity) {
		// activity may be null, to clear it
		cgss_android_set_host_jni_context(g_jvm, activity);
	}

	void JNICALL setReuseSharedWindow(JNIEnv*, jclass, jboolean enabled) {
		cgss_android_set_reuse_shared_window(enabled == JNI_TRUE ? 1 : 0);
	}

	jboolean JNICALL isReuseSharedWindowEnabled(JNIEnv*, jclass) {
		return cgss_android_is_reuse_shared_window_enabled() ? JNI_TRUE : JNI_FALSE;
	}

	template <typename F>
	void* fn(F function) {
		return reinterpret_cast<void*>(function);
	}

	const JNINativeMethod kNatives[] = {
		{ "attach", "(Landroid/view/Surface;II)V", fn(attach) },
		{ "resized", "(II)V", fn(resized) },
		{ "detach", "()V", fn(detach) },
		{ "requestClose", "()V", fn(requestClose) },
		{ "injectTouchDown", "(IFF)V", fn(injectTouchDown) },
		{ "injectTouchMove", "(IFF)V", fn(injectTouchMove) },
		{ "injectTouchUp", "(IFF)V", fn(injectTouchUp) },
		{ "injectKeyDown", "(III)V", fn(injectKeyDown) },
		{ "injectKeyUp", "(II)V", fn(injectKeyUp) },
		{ "injectText", "(I)V", fn(injectText) },
		{ "injectFocusGained", "()V", fn(injectFocusGained) },
		{ "injectFocusLost", "()V", fn(injectFocusLost) },
		{ "injectJoystickButton", "(IIZ)V", fn(injectJoystickButton) },
		{ "injectJoystickAxis", "(IFFFFFFFF)V", fn(injectJoystickAxis) },
		{ "injectJoystickConnected", "(I)V", fn(injectJoystickConnected) },
		{ "injectJoystickDisconnected", "(I)V", fn(injectJoystickDisconnected) },
		{ "nativeSetVirtualKeyboardCallback", "(Z)V", fn(nativeSetVirtualKeyboardCallback) },
		{ "nativeSetHostActivity", "(Landroid/app/Activity;)V", fn(nativeSetHostActivity) },
		{ "setReuseSharedWindow", "(Z)V", fn(setReuseSharedWindow) },
		{ "isReuseSharedWindowEnabled", "()Z", fn(isReuseSharedWindowEnabled) },
	};

} // namespace

extern "C" int cgss_android_register_natives(JNIEnv* env, jclass hostClass) {
	if (env == nullptr || hostClass == nullptr) {
		return JNI_ERR;
	}
	if (env->GetJavaVM(&g_jvm) != JNI_OK) {
		return JNI_ERR;
	}
	jmethodID dispatch = env->GetStaticMethodID(hostClass, "dispatchVirtualKeyboardRequest", "(Z)V");
	if (dispatch == nullptr) {
		return JNI_ERR; // NoSuchMethodError pending
	}
	jint rc = env->RegisterNatives(hostClass, kNatives, sizeof(kNatives) / sizeof(kNatives[0]));
	if (rc != JNI_OK) {
		return rc;
	}
	// Pinned for the process: the IME thunk runs on threads with no class
	// loader to find it. A thunk seeing the class sees the method ID too.
	g_dispatchVirtualKeyboard = dispatch;
	jclass previous = g_hostClass.exchange(static_cast<jclass>(env->NewGlobalRef(hostClass)),
	                                       std::memory_order_release);
	if (previous != nullptr) {
		env->DeleteGlobalRef(previous);
	}
	return JNI_OK;
}

extern "C" void cgss_android_set_host_surface_listener(cgss_android_host_surface_listener_t listener) {
	g_listener.store(listener);
}

#else

extern "C" void cgss_android_set_host_surface_listener(cgss_android_host_surface_listener_t) {}

#endif
