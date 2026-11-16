#include "LiteCGSS/Views/DisplayWindow.h"

#include <cmath>
#include <cassert>
#include <thread>
#include "Backend/ActiveBackend.h"
#include "LiteCGSS/Configuration/DisplayWindowSettings.h"
#include "Views/LoggerViews.h"

// Backend-neutral: every native interaction goes through B::Ops.

SKA_LOGC_CONFIG(ska::LogLevel::Info, cgss::DisplayWindow)

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Debug, cgss::DisplayWindow)
#define LOG_INFO SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Info, cgss::DisplayWindow)
#define LOG_WARN SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Warn, cgss::DisplayWindow)

#ifdef CGSS_WINDOW_PRESERVATION
static cgss::backend::ActiveBackend::native_window& sharedGameWindow() {
	static cgss::backend::ActiveBackend::native_window instance;
	return instance;
}
#endif

template <class B>
cgss::basic_DisplayWindow<B>::basic_DisplayWindow(const DisplayWindowSettings& settings) :
#ifdef CGSS_WINDOW_PRESERVATION
	m_gameWindow(sharedGameWindow()),
#else
	m_ownedGameWindow(std::make_unique<typename B::native_window>()),
	m_gameWindow(*m_ownedGameWindow),
#endif
	m_snapshot{ *this },
	m_userRenderSprite(std::make_unique<typename B::native_sprite>()),
	m_userRenderTexture(std::make_unique<typename B::native_render_texture>()) {
#ifdef CGSS_WINDOW_PRESERVATION
	reload(settings, true);
#else
	initUserRender();
	reload(settings, true);
#endif
}

template <class B>
cgss::basic_DisplayWindow<B>::basic_DisplayWindow() :
#ifdef CGSS_WINDOW_PRESERVATION
	m_gameWindow(sharedGameWindow()),
#else
	m_ownedGameWindow(std::make_unique<typename B::native_window>()),
	m_gameWindow(*m_ownedGameWindow),
#endif
	m_snapshot{ *this },
	m_userRenderSprite(std::make_unique<typename B::native_sprite>()),
	m_userRenderTexture(std::make_unique<typename B::native_render_texture>()) {
#ifndef CGSS_WINDOW_PRESERVATION
	initUserRender();
#endif
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

	LOG_DEBUG << "applyReload: force=" << force << " windowCreateNeeded=" << windowCreateNeeded;

	m_settings = std::move(settings);

	if (m_settings.headless) {
		B::Ops::window_close(m_gameWindow);
	} else {
		const auto scaledWidth = static_cast<unsigned int>(m_settings.video.width * m_settings.video.scale);
		const auto scaledHeight = static_cast<unsigned int>(m_settings.video.height * m_settings.video.scale);
		if (windowCreateNeeded) {
#ifdef CGSS_WINDOW_PRESERVATION
			if (B::Ops::window_is_open(m_gameWindow)) {
				// Window already exists (shared static), just update settings.
				// Don't call setSize() — on Android the native window is always fullscreen,
				// and setSize() would desync SFML's internal size from the actual surface.
				LOG_WARN << "Reusing existing window instead of recreating";
				B::Ops::window_set_title(m_gameWindow, m_settings.title.c_str());
			} else
#endif
			{
				const cgss::backend::WindowCreateOptions opts {
					scaledWidth,
					scaledHeight,
					static_cast<unsigned int>(m_settings.video.bitsPerPixel),
					m_settings.fullscreen,
					m_settings.vSync,
					&m_settings.context,
				};
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

	LOG_WARN << "Reloading window with parameters screenwidth = " << m_draw.screenWidth << " screenheight = " << m_draw.screenHeight
		<< " smoothScreen = " << m_draw.smoothScreen << " scale = " << m_draw.scale << " framerate = " << m_draw.frameRate;
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
void cgss::basic_DisplayWindow<B>::stop() {
	m_snapshot.stop();
#ifdef CGSS_WINDOW_PRESERVATION
	// Don't close the shared window — just clear drawables.
	// The GL context stays valid so member destructors can clean up GL resources safely.
	drawables().clear();
	B::Ops::window_clear(m_gameWindow);
	B::Ops::window_display(m_gameWindow);
#else
	B::Ops::window_close(m_gameWindow);
#endif
}

template <class B>
void cgss::basic_DisplayWindow<B>::setActive(bool active) {
	B::Ops::window_set_active(m_gameWindow, active);
}

template <class B>
void cgss::basic_DisplayWindow<B>::draw() {
	using Ops = typename B::Ops;
	applyPendingReload();
	Ops::window_clear(m_gameWindow);

	cgss::Camera defview{Ops::window_get_default_view(m_gameWindow)};
	auto render_target = configureAndGetRenderTarget(defview);

	// Rendering C++ sprite stack
	drawables().draw(defview, render_target);

	postProcessing();

	Ops::window_display(m_gameWindow);
}

template <class B>
bool cgss::basic_DisplayWindow<B>::popEvent(cgss::Event& event) {
	using Ops = typename B::Ops;
	using cgss::backend::native_of;
	return m_eventPolicyPolling
		? Ops::window_poll_event(m_gameWindow, native_of(event))
		: Ops::window_wait_event(m_gameWindow, native_of(event));
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
	LOG_INFO << "Drawing brightness " << static_cast<int>(m_draw.brightness) << " on window size " << size.x << ", " << size.y;
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
	/* VSYNC choice */
	B::Ops::window_set_vsync(m_gameWindow, m_settings.vSync);
	B::Ops::window_set_framerate_limit(m_gameWindow, !m_settings.vSync ? m_settings.frameRate : 0);
}

template <class B>
void cgss::basic_DisplayWindow<B>::setFrameRate(unsigned int frameRate) {
	m_settings.frameRate = frameRate;
	B::Ops::window_set_framerate_limit(m_gameWindow, !m_settings.vSync ? m_settings.frameRate : 0);
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
		LOG_INFO << "Binding render states " << &renderStates->data();
	} else {
		ShaderRenderable::bindValue(nullptr);
		setShader(nullptr);
		LOG_INFO << "Unbinding render states";
	}
}

template <class B>
void cgss::basic_DisplayWindow<B>::updateFromValue(const RenderStatesData* renderStatesData) {
	LOG_INFO << "Updating render states " << renderStatesData;
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

template class cgss::basic_DisplayWindow<cgss::backend::ActiveBackend>;
