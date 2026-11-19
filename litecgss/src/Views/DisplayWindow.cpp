#include "LiteCGSS/Views/DisplayWindow.h"
#include "Inputs/InjectionQueue.h"
#include "Views/FrameDump.h"
#include "Common/Profiling.h"

#include <cmath>
#include <cstdlib>
#include <string>
#include <cassert>
#include <atomic>
#include <mutex>
#include <thread>
#include "Backend/ActiveBackend.h"
#include "LiteCGSS/Configuration/DisplayWindowSettings.h"
#include "LiteCGSS/Platform/Android/HostSurface.h"
#include "Common/Log.h"
#include "Views/GLContext.h"

// Backend-neutral: every native interaction goes through B::Ops.

CGSS_LOG_COMPONENT("DisplayWindow", info)


#ifdef CGSS_WINDOW_PRESERVATION
static cgss::backend::ActiveBackend::native_window& sharedGameWindow() {
	static cgss::backend::ActiveBackend::native_window instance;
	return instance;
}

// The preserved window survives DisplayWindow lifetimes (GL context
// included): for fullscreen settings, or when the host opted in (a hosted
// Android game restarted in the same process). Otherwise the window is
// owned and destroyed with the DisplayWindow (a splash / loader leaving a
// clean state behind).
static bool usesSharedGameWindow(const cgss::DisplayWindowSettings& settings) {
	return settings.fullscreen || cgss::android::isReuseSharedWindowEnabled();
}
#endif

namespace {
	// LITECGSS_NO_VSYNC=1: the display has no vertical sync to wait for (a
	// virtual X server, e.g. an agent run's Xvfb), so a game asking for vsync
	// would run uncapped; its frame rate caps it instead.
	bool vsyncAvailable() {
		static const bool available = [] {
			const char* value = std::getenv("LITECGSS_NO_VSYNC");
			return value == nullptr || *value == '\0' || std::string{ value } == "0";
		}();
		return available;
	}

	// The frame-rate cap: none under vsync; the game's frame rate otherwise.
	// A game that asked for vsync where there is none (frame rate 0, "vsync
	// paces me") gets the 60 Hz vsync would have given it.
	unsigned int frameLimit(bool vsyncRequested, unsigned int frameRate) {
		if (vsyncRequested && vsyncAvailable()) return 0;
		if (vsyncRequested && frameRate == 0) return 60;
		return frameRate;
	}
}

template <class B>
cgss::basic_DisplayWindow<B>::basic_DisplayWindow(const DisplayWindowSettings& settings) :
	m_ownedGameWindow(std::make_unique<typename B::native_window>()),
#ifdef CGSS_WINDOW_PRESERVATION
	m_gameWindow(usesSharedGameWindow(settings) ? sharedGameWindow() : *m_ownedGameWindow),
#else
	m_gameWindow(*m_ownedGameWindow),
#endif
	m_snapshot{ *this },
	m_userRenderSprite(std::make_unique<typename B::native_sprite>()),
	m_userRenderTexture(std::make_unique<typename B::native_render_texture>()) {
	if (!usesSharedWindow()) {
		initUserRender();
	}
	reload(settings, true);
}

// No settings, so no fullscreen to ask for the preserved window: owned.
template <class B>
cgss::basic_DisplayWindow<B>::basic_DisplayWindow() :
	m_ownedGameWindow(std::make_unique<typename B::native_window>()),
	m_gameWindow(*m_ownedGameWindow),
	m_snapshot{ *this },
	m_userRenderSprite(std::make_unique<typename B::native_sprite>()),
	m_userRenderTexture(std::make_unique<typename B::native_render_texture>()) {
	initUserRender();
}

template <class B>
cgss::basic_DisplayWindow<B>::~basic_DisplayWindow() {
	stop();
}

template <class B> cgss::basic_DisplayWindowDrawData<B>::basic_DisplayWindowDrawData() = default;
template <class B> cgss::basic_DisplayWindowDrawData<B>::~basic_DisplayWindowDrawData() = default;
template <class B> cgss::basic_DisplayWindowDrawData<B>::basic_DisplayWindowDrawData(basic_DisplayWindowDrawData&&) noexcept = default;
template <class B> cgss::basic_DisplayWindowDrawData<B>& cgss::basic_DisplayWindowDrawData<B>::operator=(basic_DisplayWindowDrawData&&) noexcept = default;
template class cgss::basic_DisplayWindowDrawData<cgss::backend::ActiveBackend>;

template <class B>
void cgss::basic_DisplayWindow<B>::initUserRender() {
	using Ops = typename B::Ops;
	Ops::render_texture_create(*m_userRenderTexture, m_draw.screenWidth, m_draw.screenHeight);
	Ops::render_texture_set_smooth(*m_userRenderTexture, m_draw.smoothScreen);

	Ops::set_sprite_texture(*m_userRenderSprite,
		Ops::render_texture_get_texture(*m_userRenderTexture), false);
}

template <class B>
void cgss::basic_DisplayWindow<B>::reload(DisplayWindowSettings settings, bool force) {
	if (force) {
		// During initialization (force=true), no draw loop is running yet,
		// so we apply immediately on the current thread.
		applyReload(std::move(settings), force);
	} else {
		// Defer to the draw thread to avoid cross-thread OpenGL context issues
		// (critical on Android where GL ES contexts are strictly thread-bound).
		std::lock_guard<std::mutex> lock(m_pendingReloadMutex);
		m_pendingReloadSettings = std::move(settings);
		m_pendingReloadForce = false;
	}
}

template <class B>
void cgss::basic_DisplayWindow<B>::applyPendingReload() {
	std::optional<DisplayWindowSettings> settings;
	bool force = false;
	{
		std::lock_guard<std::mutex> lock(m_pendingReloadMutex);
		if (!m_pendingReloadSettings.has_value()) {
			return;
		}
		settings = std::move(m_pendingReloadSettings);
		force = m_pendingReloadForce;
		m_pendingReloadSettings.reset();
	}
	applyReload(std::move(*settings), force);
}

template <class B>
void cgss::basic_DisplayWindow<B>::applyReload(DisplayWindowSettings settings, bool force) {
	const auto windowCreateNeeded =
		force ||
		settings.video.bitsPerPixel != m_settings.video.bitsPerPixel ||
		settings.fullscreen != m_settings.fullscreen;

	CGSS_LOG_DEBUG("applyReload: force={} windowCreateNeeded={}", force, windowCreateNeeded);

	// The origin is the host's (setOrigin), not a reload parameter: settings
	// coming from the scripts never carry it
	settings.originX = m_settings.originX;
	settings.originY = m_settings.originY;
	m_settings = std::move(settings);

	if (m_settings.headless) {
		B::Ops::window_close(m_gameWindow);
		cgss::android::hostSurfaceEventsDropped();
	} else {
		const auto scaledWidth = static_cast<unsigned int>(m_settings.video.width * m_settings.video.scale);
		const auto scaledHeight = static_cast<unsigned int>(m_settings.video.height * m_settings.video.scale);
		if (windowCreateNeeded) {
#ifdef CGSS_WINDOW_PRESERVATION
			if (B::Ops::window_is_open(m_gameWindow)) {
				// Window already exists (shared static), just update settings.
				// Don't call setSize() — on Android the native window is always fullscreen,
				// and resizing would desync the backend's internal size from the actual surface.
				CGSS_LOG_WARN("Reusing existing window instead of recreating");
				// As a recreated one: the closes made for the previous
				// DisplayWindow don't reach this one
				cgss::android::hostSurfaceEventsDropped();
				B::Ops::window_set_title(m_gameWindow, m_settings.title.c_str());
			} else
#endif
			{
				const cgss::backend::WindowCreateOptions opts {
					scaledWidth,
					scaledHeight,
					static_cast<unsigned int>(m_settings.video.bitsPerPixel),
					m_settings.fullscreen,
					m_settings.vSync && vsyncAvailable(),
					m_settings.hidden,
					&m_settings.context,
				};
				// The old window goes first: a close requested while the new
				// one is created lands in its queue, and is counted
				B::Ops::window_close(m_gameWindow);
				cgss::android::hostSurfaceEventsDropped();
				B::Ops::window_create(m_gameWindow, opts, m_settings.title.c_str());
			}
		} else {
			B::Ops::window_set_size(m_gameWindow, scaledWidth, scaledHeight);
			B::Ops::window_set_title(m_gameWindow, m_settings.title.c_str());
		}
		B::Ops::window_set_mouse_cursor_visible(m_gameWindow, m_settings.visibleMouse);
		setVsync(m_settings.vSync);
	}

	m_draw.smoothScreen = m_settings.smoothScreen;
	m_draw.scale = m_settings.video.scale;
	m_draw.frameRate = m_settings.frameRate;

	m_snapshot.init();

	const auto windowSizeChanged = force ||
		m_draw.screenWidth != static_cast<long>(m_settings.video.width) ||
		m_draw.screenHeight != static_cast<long>(m_settings.video.height);
	if (windowSizeChanged) {
		onSizeChanged();
	}

	CGSS_LOG_WARN("Reloading window with parameters screenwidth = {} screenheight = {} smoothScreen = {} scale = {} framerate = {}", m_draw.screenWidth, m_draw.screenHeight, m_draw.smoothScreen, m_draw.scale, m_draw.frameRate);
}

template <class B>
void cgss::basic_DisplayWindow<B>::onSizeChanged() {
	m_draw.screenWidth = m_settings.video.width;
	m_draw.screenHeight = m_settings.video.height;

	/* Render resize */
	if (m_draw.renderTexture != nullptr) {
		B::Ops::render_texture_create(*m_draw.renderTexture, m_draw.screenWidth, m_draw.screenHeight);
	}

	/* Re-init user drawing surface */
	initUserRender();

	notifyWindowSizeChange();
}

template <class B>
void cgss::basic_DisplayWindow<B>::notifyWindowSizeChange() {
	/* This event will help by notifying subscribed viewboxes (Viewports / FramedViews / ...) */
	auto we = WindowSizeChangeEvent{};
	we.width = m_settings.video.width;
	we.height = m_settings.video.height;
	we.originX = m_settings.originX;
	we.originY = m_settings.originY;
	m_eventDispatcher.Observable<WindowSizeChangeEvent>::notifyObservers(we);
}

template <class B>
cgss::RenderTarget cgss::basic_DisplayWindow<B>::configureAndGetRenderTarget(cgss::Camera& defview) {
	using Ops = typename B::Ops;
	using cgss::backend::native_of;

	// Setting the default view parameters
	Ops::view_set_size(native_of(defview), static_cast<float>(m_draw.screenWidth), static_cast<float>(m_draw.screenHeight));
	Ops::view_set_center(native_of(defview), round(m_draw.screenWidth / 2.0f), round(m_draw.screenHeight / 2.0f));

	// Appying the default view to the Window (used in postProcessing())
	Ops::window_set_view(m_gameWindow, native_of(defview));

	// If the m_renderTexture is defined, we use it instead of the m_gameWindow (shader processing)
	if (m_draw.renderTexture != nullptr) {
		// Set the default view + clear via backend ops
		Ops::render_texture_set_view(*m_draw.renderTexture, native_of(defview));
		Ops::render_texture_clear(*m_draw.renderTexture);
		return cgss::RenderTarget{*m_draw.renderTexture.get()};
	}
	return cgss::RenderTarget{Ops::target_from_window(m_gameWindow)};
}

template <class B>
cgss::DisplayWindowContextSettings cgss::basic_DisplayWindow<B>::getContextSettings() const {
	return B::Ops::window_get_settings(m_gameWindow);
}

template <class B>
const cgss::DisplayWindowSettings& cgss::basic_DisplayWindow<B>::getSettings() const {
	return m_settings;
}

template <class B>
bool cgss::basic_DisplayWindow<B>::isOpen() const {
	return B::Ops::window_is_open(m_gameWindow);
}

template <class B>
bool cgss::basic_DisplayWindow<B>::usesSharedWindow() const {
	return &m_gameWindow != m_ownedGameWindow.get();
}

template <class B>
void cgss::basic_DisplayWindow<B>::stop() {
	m_snapshot.stop();
	if (usesSharedWindow()) {
		// A close this DisplayWindow never read was meant for it, not for
		// the next one the preserved window serves
		cgss::android::hostSurfaceEventsDropped();
		// The preserved window stays open for the next DisplayWindow, left
		// clean. That's GL work: skipped if the hosted surface is gone.
		if (!cgss::android::beginHostedDraw()) {
			// The context may still be current on this (exiting) thread:
			// release it, or the next render thread can't make it current.
			B::Ops::window_set_active(m_gameWindow, false);
			return;
		}
		drawables().clear();
		B::Ops::window_clear(m_gameWindow);
		B::Ops::window_display(m_gameWindow);
		// Same: the context survives this DisplayWindow and its thread.
		B::Ops::window_set_active(m_gameWindow, false);
		cgss::android::endHostedDraw();
		return;
	}
	// Owned window: closed with its DisplayWindow (its queue with it).
	B::Ops::window_close(m_gameWindow);
	cgss::android::hostSurfaceEventsDropped();
}

template <class B>
void cgss::basic_DisplayWindow<B>::setActive(bool active) {
	// Back from the background, the GL surface is rebuilt by the event
	// pump: activating before can only fail. The next frame activates it.
	if (active && m_pumpPending) {
		return;
	}
	B::Ops::window_set_active(m_gameWindow, active);
}

template <class B>
void cgss::basic_DisplayWindow<B>::draw() {
	CGSS_PROFILE_ZONE("cgss::DisplayWindow::draw");
	using Ops = typename B::Ops;
	// Hosted surface (Android; always passes elsewhere): no GL work while
	// it is detached, the frame waiting for it to come back (the game
	// pauses in the background) or for a close request. A detach can't
	// happen during the frame. Back from it, nothing is drawn until the
	// event pump has rebuilt the GL surface.
	// A window that doesn't own the screen (a backend with one OS window,
	// another window on top of it) has nothing to draw.
	if (!Ops::window_is_drawable(m_gameWindow)) {
		return;
	}
	bool surfaceChanged = false;
	if (!cgss::android::beginHostedFrame(m_hostSurfaceDetaches, surfaceChanged)) {
		if (surfaceChanged) m_pumpPending = true;
		return;
	}
	if (m_pumpPending) {
		cgss::android::endHostedDraw();
		return;
	}
	applyPendingReload();
	Ops::window_clear(m_gameWindow);

	cgss::Camera defview{Ops::window_get_default_view(m_gameWindow)};
	auto render_target = configureAndGetRenderTarget(defview);

	// Rendering C++ sprite stack. The drawables placed on the window itself are
	// moved by the origin like the viewports are (viewports set their own view);
	// post processing keeps the unmoved default view.
	cgss::Camera gameView{defview};
	Ops::view_set_center(cgss::backend::native_of(gameView),
		round(m_draw.screenWidth / 2.0f) - static_cast<float>(m_settings.originX),
		round(m_draw.screenHeight / 2.0f) - static_cast<float>(m_settings.originY));
	drawables().draw(gameView, render_target);

	postProcessing();

	// The presented frame, out of band (LITECGSS_DUMP_FRAMES; dev builds only).
	cgss::detail::frameDumpAfterDraw(this, m_settings.video.scale, [this] { return takeSnapshot(); });

	Ops::window_display(m_gameWindow);
	CGSS_PROFILE_FRAME();

	cgss::android::endHostedDraw();
}

template <class B>
bool cgss::basic_DisplayWindow<B>::popEvent(cgss::Event& event) {
	using Ops = typename B::Ops;
	using cgss::backend::native_of;
	// Not gated on the hosted surface: the queue survives a detach, and the
	// focus pair a detach / attach pushes must be pumped (the backend drops
	// and rebuilds the GL surface from them).
	// Injected input (cgss::input::injectKey) comes first, as if the device
	// had sent it just now.
	cgss::KeyEvent injected;
	bool injectedDown = false;
	if (cgss::input::detail::popInjectedKey(injected, injectedDown)) {
		Ops::event_set_key(native_of(event), injectedDown, injected);
		return true;
	}
	bool popped = Ops::window_poll_event(m_gameWindow, native_of(event));
	if (!popped) {
		// Drained: the backend handled its window events (the GL surface rebuild)
		m_pumpPending = false;
		popped = !m_eventPolicyPolling && Ops::window_wait_event(m_gameWindow, native_of(event));
	}
	if (popped && event.type() == cgss::EventType::Closed) {
		cgss::android::hostSurfaceCloseDelivered();
	}
	if (popped && event.type() == cgss::EventType::LostFocus) {
		cgss::android::hostSurfaceFocusLostDelivered();
	}
	return popped;
}

template <class B>
void cgss::basic_DisplayWindow<B>::changeEventPolicy(bool polling) {
	m_eventPolicyPolling = polling;
}

template <class B>
void cgss::basic_DisplayWindow<B>::drawBrightness() {
	using Ops = typename B::Ops;
	const auto size = Ops::window_get_size(m_gameWindow);
	const cgss::Color overlay{ 0, 0, 0, static_cast<std::uint8_t>(255 - m_draw.brightness) };
	CGSS_LOG_INFO("Drawing brightness {} on window size {}, {}", static_cast<int>(m_draw.brightness), size.x, size.y);
	Ops::window_draw_color_quad(m_gameWindow, 0.f, 0.f,
		static_cast<float>(size.x), static_cast<float>(size.y), overlay);
}

template <class B>
void cgss::basic_DisplayWindow<B>::postProcessing() {
	using Ops = typename B::Ops;
	using cgss::backend::native_of;

	// Drawing render to window if finished
	if (m_draw.renderTexture) {
		Ops::render_texture_display(*m_draw.renderTexture);
		const auto* states = m_draw.renderState == nullptr
			? nullptr
			: &native_of(*m_draw.renderState);
		Ops::window_draw_render_texture(m_gameWindow, *m_draw.renderTexture, states);
	}

	//Draw the "freeze" texture, if visible
	m_snapshot.draw(m_gameWindow);

	// Update the brightness (applied to m_gameWindow)
	if (m_draw.brightness != 255) {
		drawBrightness();
	}
}

template <class B>
void cgss::basic_DisplayWindow<B>::setOrigin(long x, long y) {
	if (m_settings.originX == x && m_settings.originY == y) {
		return;
	}
	m_settings.originX = x;
	m_settings.originY = y;
	CGSS_LOG_INFO("Window origin set to x = {} y = {}", x, y);
	notifyWindowSizeChange();
}

template <class B>
cgss::IntPoint cgss::basic_DisplayWindow<B>::originPixels() const {
	if ((m_settings.originX == 0 && m_settings.originY == 0) || m_draw.screenWidth <= 0 || m_draw.screenHeight <= 0) {
		return {};
	}
	const auto size = B::Ops::window_get_size(m_gameWindow);
	return {
		static_cast<int>(std::lround(static_cast<double>(m_settings.originX) * size.x / m_draw.screenWidth)),
		static_cast<int>(std::lround(static_cast<double>(m_settings.originY) * size.y / m_draw.screenHeight))
	};
}

template <class B>
void cgss::basic_DisplayWindow<B>::resizeScreen(unsigned int width, unsigned int height) {
	m_settings.video.width = width;
	m_settings.video.height = height;

	const auto scaledWidth = static_cast<unsigned int>(m_settings.video.width * m_settings.video.scale);
	const auto scaledHeight = static_cast<unsigned int>(m_settings.video.height * m_settings.video.scale);
	B::Ops::window_set_size(m_gameWindow, scaledWidth, scaledHeight);

	onSizeChanged();
}

template <class B>
void cgss::basic_DisplayWindow<B>::setTitle(const char* title) {
	m_settings.title = title;
	B::Ops::window_set_title(m_gameWindow, m_settings.title.c_str());
}

template <class B>
void cgss::basic_DisplayWindow<B>::setVsync(bool enable) {
	m_settings.vSync = enable;
	/* VSYNC choice: the settings keep what the game asked for; without vsync
	   available, the frame rate is the cap. */
	const bool vsync = m_settings.vSync && vsyncAvailable();
	B::Ops::window_set_vsync(m_gameWindow, vsync);
	B::Ops::window_set_framerate_limit(m_gameWindow, frameLimit(m_settings.vSync, m_settings.frameRate));
}

template <class B>
void cgss::basic_DisplayWindow<B>::setFrameRate(unsigned int frameRate) {
	m_settings.frameRate = frameRate;
	B::Ops::window_set_framerate_limit(m_gameWindow, frameLimit(m_settings.vSync, m_settings.frameRate));
}

template <class B>
void cgss::basic_DisplayWindow<B>::setScale(double scale) {
	m_settings.video.scale = scale;

	const auto scaledWidth = static_cast<unsigned int>(m_settings.video.width * m_settings.video.scale);
	const auto scaledHeight = static_cast<unsigned int>(m_settings.video.height * m_settings.video.scale);
	B::Ops::window_set_size(m_gameWindow, scaledWidth, scaledHeight);
}

template <class B>
void cgss::basic_DisplayWindow<B>::setIcon(const cgss::Image& image) {
	using Ops = typename B::Ops;
	using cgss::backend::native_of;
	const auto& raw = native_of(image);
	Ops::window_set_icon(m_gameWindow, image.width(), image.height(), Ops::get_image_pixels_ptr(raw));
}

template <class B>
void cgss::basic_DisplayWindow<B>::setShader(const cgss::DrawState* renderStates) {
	using Ops = typename B::Ops;
	m_draw.renderState = renderStates;
	if (m_draw.renderTexture == nullptr && m_draw.renderState != nullptr) {
		m_draw.renderTexture = std::make_unique<typename B::native_render_texture>();
		Ops::render_texture_create(*m_draw.renderTexture, m_draw.screenWidth, m_draw.screenHeight);
	}
}

template <class B>
void cgss::basic_DisplayWindow<B>::bindRenderStates(RenderStates* renderStates) {
	if (renderStates != nullptr) {
		renderStates->data().setShaderUniform("texture", cgss::CurrentTexture);
		ShaderRenderable::bindValue(&renderStates->data());
		setShader(&renderStates->data().getRenderStates());
		CGSS_LOG_INFO("Binding render states {}", static_cast<const void*>(&renderStates->data()));
	} else {
		ShaderRenderable::bindValue(nullptr);
		setShader(nullptr);
		CGSS_LOG_INFO("Unbinding render states");
	}
}

template <class B>
void cgss::basic_DisplayWindow<B>::updateFromValue(const RenderStatesData* renderStatesData) {
	CGSS_LOG_INFO("Updating render states {}", static_cast<const void*>(renderStatesData));
	setShader(renderStatesData == nullptr ? nullptr : &renderStatesData->getRenderStates());
}

template <class B>
std::unique_ptr<cgss::Texture> cgss::basic_DisplayWindow<B>::takeSnapshot() const {
	return m_snapshot.takeSnapshot(m_gameWindow);
}

template <class B>
int cgss::basic_DisplayWindow<B>::getX() const {
	return B::Ops::window_get_position(m_gameWindow).x;
}

template <class B>
int cgss::basic_DisplayWindow<B>::getY() const {
	return B::Ops::window_get_position(m_gameWindow).y;
}

template <class B>
void cgss::basic_DisplayWindow<B>::transition(long time, Texture* texture) {
	m_snapshot.transition(time, texture);
}

template <class B>
void cgss::basic_DisplayWindow<B>::freeze() {
	m_snapshot.freeze(m_gameWindow);
}

template <class B>
void cgss::basic_DisplayWindow<B>::move(int x, int y) {
	B::Ops::window_set_position(m_gameWindow, x, y);
}

template <class B>
unsigned int cgss::basic_DisplayWindow<B>::DesktopWidth() {
	return B::Ops::desktop_size().x;
}

template <class B>
unsigned int cgss::basic_DisplayWindow<B>::DesktopHeight() {
	return B::Ops::desktop_size().y;
}

template <class B>
std::vector<cgss::VideoMode> cgss::basic_DisplayWindow<B>::FullscreenModes() {
	return B::Ops::fullscreen_modes();
}

namespace {
	// Held by the thread whose GL work is in progress (context_follows_thread)
	std::mutex g_contextMutex;
	thread_local bool t_holdsContext = false;
	// Threads take the context on their first GL work (ensureGLContext), not
	// before (AcquireContextOnUse)
	std::atomic<bool> g_contextOnUse{ false };
}

void cgss::detail::ensureGLContext() {
	if (t_holdsContext || !g_contextOnUse.load(std::memory_order_relaxed)) return;
	cgss::DisplayWindow::AcquireContext();
}

template <class B>
void cgss::basic_DisplayWindow<B>::AcquireContextOnUse(bool onUse) {
	if (!B::Ops::context_follows_thread) return;
	g_contextOnUse.store(onUse, std::memory_order_relaxed);
}

template <class B>
bool cgss::basic_DisplayWindow<B>::ContextFollowsThread() {
	return B::Ops::context_follows_thread;
}

template <class B>
void cgss::basic_DisplayWindow<B>::AcquireContext() {
	if (!B::Ops::context_follows_thread || t_holdsContext) return;
	g_contextMutex.lock();
	t_holdsContext = true;
	B::Ops::context_make_current(true);
}

template <class B>
void cgss::basic_DisplayWindow<B>::ReleaseContext() {
	if (!B::Ops::context_follows_thread || !t_holdsContext) return;
	B::Ops::context_make_current(false);
	t_holdsContext = false;
	g_contextMutex.unlock();
}

template class cgss::basic_DisplayWindow<cgss::backend::ActiveBackend>;
