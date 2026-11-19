#ifndef CGSS_DISPLAY_WINDOW_H
#define CGSS_DISPLAY_WINDOW_H

#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <vector>

#include "LiteCGSS/Backend/ActiveBackendFwd.h"
#include "LiteCGSS/Common/Point.h"
#include "LiteCGSS/Configuration/VideoMode.h"
#include "LiteCGSS/Events/Event.h"
#include "LiteCGSS/Views/View.h"
#include "LiteCGSS/Views/ViewAuthorizations.h"
#include "LiteCGSS/Views/DisplayWindowDraw.h"
#include "LiteCGSS/Views/DisplayWindowFwd.h"
#include "LiteCGSS/Views/DisplayWindowSnapshot.h"
#include "LiteCGSS/Views/DisplayWindowUserData.h"
#include "LiteCGSS/Views/FramedView.h"
#include "LiteCGSS/Views/SnapshotCapturable.h"
#include "LiteCGSS/Graphics/ShapeFwd.h"
#include "LiteCGSS/Graphics/SpriteFwd.h"
#include "LiteCGSS/Image/Image.h"

#include "LiteCGSS/Events/MainEventDispatcher.h"
#include "LiteCGSS/Events/Patterns/SubObserver.h"
#include "LiteCGSS/Events/Patterns/Observable.h"
#include "LiteCGSS/Configuration/DisplayWindowSettings.h"

namespace cgss {

	struct DisplayWindowSettings;

	// Templated wrapper over `Backend::native_window` (+ render texture +
	// user-render sprite). Consumers refer to it as `cgss::DisplayWindow`.
	//
	// The body in DisplayWindow.cpp is backend-neutral: every native
	// interaction goes through `B::Ops`. Full-class specialization in a
	// backend's Specializations folder remains the escape hatch when a
	// wrapper's behavior diverges radically per backend.
	template <class B>
	class basic_DisplayWindow :
		public View,
		public SnapshotCapturable,
		public ShaderRenderable {

	public:
		static unsigned int DesktopWidth();
		static unsigned int DesktopHeight();
		// Fullscreen modes of the primary display, best first.
		static std::vector<cgss::VideoMode> FullscreenModes();

		/* GL context and threads. Some backends (raylib) have one GL context
		   for all their windows: it is current on one thread at a time, so a
		   host driving windows from several threads hands it over: a thread
		   about to do GL work (draw, create textures...) acquires it, and
		   releases it before another thread can go on. ContextFollowsThread()
		   tells whether that's needed; elsewhere (SFML: one context per
		   window, activated by the backend) both calls do nothing.
		   AcquireContext blocks while another thread holds it; both are
		   no-ops when the calling thread already holds / doesn't hold it. */
		static bool ContextFollowsThread();
		static void AcquireContext();
		static void ReleaseContext();
		/* With onUse, a thread takes the context on its first GL work (the
		   backend's GL operations ask for it), not before: a host handing it
		   over with its own thread lock (Ruby's GVL) releases it with that
		   lock, but doesn't acquire it with it. A thread doing no GL work
		   (file I/O, script compilation) then never waits for a frame drawn
		   on another thread. A no-op where the context doesn't follow
		   threads. */
		static void AcquireContextOnUse(bool onUse);

		basic_DisplayWindow(const DisplayWindowSettings& settings);

		basic_DisplayWindow(const basic_DisplayWindow&) = delete;
		basic_DisplayWindow& operator=(basic_DisplayWindow&) = delete;
		basic_DisplayWindow(basic_DisplayWindow&&) = delete;
		basic_DisplayWindow& operator=(basic_DisplayWindow&&) = delete;

		basic_DisplayWindow();
		virtual ~basic_DisplayWindow();

		unsigned char brightness() const { return m_draw.brightness; }
		long screenWidth() const { return m_draw.screenWidth; }
		long screenHeight() const { return m_draw.screenHeight; }
		bool smoothScreen() const { return m_draw.smoothScreen; }
		double scale() const { return m_draw.scale; }
		auto frameRate() const { return m_draw.frameRate; }
		int getX() const;
		int getY() const;

		void stop();
		bool isOpen() const;
		void draw();
		bool popEvent(cgss::Event& event);
		void setActive(bool active);
		void changeEventPolicy(bool polling);

		std::unique_ptr<cgss::Texture> takeSnapshot() const override;
		void freeze();
		void transition(long time, Texture* texture = nullptr);

		void setBrightness(unsigned char brightness) { m_draw.brightness = brightness; }
		void bindRenderStates(RenderStates* states);
		void setIcon(const cgss::Image& image);
		void move(int x, int y);

		/* Settings related stuff */
		void reload(DisplayWindowSettings settings, bool force = false);
		const DisplayWindowSettings& getSettings() const;
		DisplayWindowContextSettings getContextSettings() const;
		void resizeScreen(unsigned int width, unsigned int height);
		/* Where the game's (0, 0) is on the window (DisplayWindowSettings::originX) */
		void setOrigin(long x, long y);
		long originX() const { return m_settings.originX; }
		long originY() const { return m_settings.originY; }
		/* The origin in the window's own pixels: the unit of its mouse and touch events */
		cgss::IntPoint originPixels() const;
		void setVsync(bool enable);
		void setTitle(const char* title);
		void setFrameRate(unsigned int frameRate);
		void setScale(double scale);

		template <class View, class ... Args>
		View addView(Args&& ... args) {
			return View::create(*this, m_eventDispatcher, buildUserData(), m_settings, std::forward<Args>(args)...);
		}

		template <class Event>
		void record(Observer<Event>& observer) {
			m_eventDispatcher.template Observable<Event>::addObserver(observer);
		}

		template <class Event>
		void erase(Observer<Event>& observer) {
			m_eventDispatcher.template Observable<Event>::removeObserver(observer);
		}

		MainEventDispatcher& eventDispatcher() { return m_eventDispatcher; }

	private:
		DisplayWindowUserData buildUserData() {
			return { m_eventDispatcher, m_settings, m_userRenderSprite, m_userRenderTexture };
		}
		void applyReload(DisplayWindowSettings settings, bool force);
		void applyPendingReload();
		void onSizeChanged();
		void notifyWindowSizeChange();

		void initUserRender();
		void drawBrightness();
		// Bound to the preserved window rather than its own (see m_gameWindow)
		bool usesSharedWindow() const;
		cgss::RenderTarget configureAndGetRenderTarget(cgss::Camera& defview);
		void postProcessing();
		void setShader(const cgss::DrawState* shader);
		void updateFromValue(const RenderStatesData* renderStatesData) override;

		bool m_eventPolicyPolling = true;
		// Hosted surface (Android): detaches seen by this window's frames,
		// and whether a frame must wait for the event pump (the GL surface
		// is rebuilt there after the surface came back).
		unsigned int m_hostSurfaceDetaches = 0;
		bool m_pumpPending = false;
		MainEventDispatcher m_eventDispatcher {};
		DisplayWindowSettings m_settings {};
		/* m_ownedGameWindow holds the owned window; m_gameWindow is the bound reference:
			to the owned one, or, in CGSS_WINDOW_PRESERVATION builds, to the process-wide
			preserved one for fullscreen settings or when the host opted in
			(cgss::android::setReuseSharedWindow) — see DisplayWindow.cpp ctors.
			Declaration order matters: m_ownedGameWindow must come before m_gameWindow. */
		std::unique_ptr<typename B::native_window> m_ownedGameWindow;
		typename B::native_window& m_gameWindow;
		// The full snapshot/transition path is implemented per backend. For
		// backends that do not implement it, swap to a no-op stub via the
		// `has_snapshot` capability tag — same call sites in the .cpp body
		// work for either branch, and no backend is named here.
		using SnapshotImpl = std::conditional_t<
			B::has_snapshot,
			DisplayWindowSnapshot,
			StubDisplayWindowSnapshot<basic_DisplayWindow<B>>>;
		SnapshotImpl m_snapshot;
		DisplayWindowDrawData m_draw;

		std::mutex m_pendingReloadMutex;
		std::optional<DisplayWindowSettings> m_pendingReloadSettings;
		bool m_pendingReloadForce = false;

		/* The user render sprite and texture belong to the window and are only used by children element on the window.
			As of today, only viewports use this. The use of render sprite and texture is to be used as the target view of
			children viewports (when render states, so shaders, are enabled).
			Initialized in the constructor (native types must remain complete in the .cpp only).
		*/
		std::unique_ptr<typename B::native_sprite> m_userRenderSprite;
		std::unique_ptr<typename B::native_render_texture> m_userRenderTexture;
	};

	class Text;
	class Viewport;

	template <>
	struct ViewAuthorizations<DisplayWindow> : public AuthorizedDrawableSet<Sprite, Text, Shape, Viewport, FramedView> {};
}

#endif
