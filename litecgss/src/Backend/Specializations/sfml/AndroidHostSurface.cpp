// SFML half of the Android host surface (Platform/Android/HostSurfaceBackend.h):
// SFML-opengl-es2's hosted Android window. Upstream SFML has no such API,
// so a hosted Android build needs that fork.

#if defined(__ANDROID__) || defined(ANDROID)

#include <SFML/Window/Android/HostedAndroidWindow.hpp>
#include <SFML/Window/Event.hpp>

#include "Platform/Android/HostSurfaceBackend.h"

namespace cgss::android::backend {

	void attach(void* nativeWindow, int width, int height) {
		// First call: creates SFML's hosted activity state and EGL display.
		// Later ones (back from the background): rebinds the native window.
		sf::android::prepareHostedActivity(nativeWindow, width, height);
	}

	void detach() {
		// The window's code finds no native window instead of a dangling one
		sf::android::clearHostedSurfaceWindow();
	}

	void resized(int width, int height) {
		sf::android::notifyHostedSurfaceResized(width, height);
	}

	void setJniContext(JavaVM* vm, jobject activity) {
		sf::android::setHostedJniContext(vm, activity);
	}

	bool pushFocus(bool gained) {
		// SFML drops (LostFocus) / rebuilds (GainedFocus) the EGL surface
		// when the window's event pump handles these.
		sf::Event event;
		event.type = gained ? sf::Event::GainedFocus : sf::Event::LostFocus;
		return sf::android::injectHostedEvent(event);
	}

	bool pushClosed() {
		sf::Event event;
		event.type = sf::Event::Closed;
		return sf::android::injectHostedEvent(event);
	}

	bool pushTouch(TouchPhase phase, int pointerId, float x, float y) {
		sf::Event event;
		event.type = phase == TouchPhase::Began ? sf::Event::TouchBegan
		           : phase == TouchPhase::Moved ? sf::Event::TouchMoved
		           : sf::Event::TouchEnded;
		event.touch.finger = static_cast<unsigned int>(pointerId);
		event.touch.x = static_cast<int>(x);
		event.touch.y = static_cast<int>(y);
		return sf::android::injectHostedEvent(event);
	}

	void pushKeyDown(int androidKeyCode, int metaState, int repeatCount) {
		sf::android::injectHostedKeyDown(androidKeyCode, metaState, repeatCount);
	}

	void pushKeyUp(int androidKeyCode, int metaState) {
		sf::android::injectHostedKeyUp(androidKeyCode, metaState);
	}

	void pushText(unsigned int unicodeCodepoint) {
		sf::android::injectHostedText(unicodeCodepoint);
	}

	void pushJoystickButton(int deviceId, int androidKeyCode, bool pressed) {
		sf::android::injectHostedJoystickButton(deviceId, androidKeyCode, pressed);
	}

	void pushJoystickAxis(int   deviceId,
	                      float axisX,    float axisY,
	                      float axisZ,    float axisRz,
	                      float hatX,     float hatY,
	                      float lTrigger, float rTrigger) {
		sf::android::injectHostedJoystickAxis(deviceId, axisX, axisY, axisZ, axisRz, hatX, hatY, lTrigger, rTrigger);
	}

	void pushJoystickConnected(int deviceId) {
		sf::android::injectHostedJoystickConnected(deviceId);
	}

	void pushJoystickDisconnected(int deviceId) {
		sf::android::injectHostedJoystickDisconnected(deviceId);
	}

}

#endif
