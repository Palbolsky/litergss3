#ifndef CGSS_DISPLAY_WINDOW_SNAPSHOT_H
#define CGSS_DISPLAY_WINDOW_SNAPSHOT_H

#include <memory>
#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Graphics/RenderStates.h"
#include "LiteCGSS/Views/DisplayWindowFwd.h"

namespace cgss {

	namespace detail {
		// Backend hook for capturing the current screen into a fresh
		// cgss::Texture. Body is defined in the per-backend
		// Backend/Specializations/<backend>/DisplayWindowSnapshot.cpp.
		std::unique_ptr<cgss::Texture> backendCaptureScreen();
	}

	// Backends without a full snapshot/transition path (has_snapshot == false)
	// route through this stub — `freeze`/`transition`/`draw` are no-ops,
	// `takeSnapshot` delegates to backendCaptureScreen so basic
	// "capture the current frame" calls (PSDK's `Graphics.snap_to_bitmap`)
	// still produce a usable bitmap. Backends with has_snapshot == true use
	// DisplayWindowSnapshot below, implemented in their Specializations folder.
	template <class DisplayWindowT>
	class StubDisplayWindowSnapshot {
	public:
		StubDisplayWindowSnapshot(DisplayWindowT&) {}
		void init() {}
		void stop() {}
		template <class W> std::unique_ptr<cgss::Texture> takeSnapshot(const W&) const {
			return detail::backendCaptureScreen();
		}
		template <class W> void freeze(const W&) {}
		void transition(long, cgss::Texture* = nullptr) {}
		template <class W> void draw(W&) {}
	};

	class DisplayWindowSnapshot {
	public:
		DisplayWindowSnapshot(DisplayWindow& window);
		~DisplayWindowSnapshot();
		// Non-copyable, non-movable: holds a reference member and is only ever
		// constructed as a value member of DisplayWindow (which is itself non-movable).
		DisplayWindowSnapshot(DisplayWindowSnapshot&&) = delete;
		DisplayWindowSnapshot& operator=(DisplayWindowSnapshot&&) = delete;
		DisplayWindowSnapshot(const DisplayWindowSnapshot&) = delete;
		DisplayWindowSnapshot& operator=(const DisplayWindowSnapshot&) = delete;
		template <class W> std::unique_ptr<cgss::Texture> takeSnapshot(const W& window) const;
		template <class W> void freeze(const W& window);
		void transition(long time, Texture* texture = nullptr);
		void stop();
		template <class W> void draw(W& window);
		void init();
	private:
		template <class W> void takeSnapshotOn(const W& window, backend::ActiveBackend::native_texture& text) const;
		void transitionBasic(long time);
		void transitionRGSS(long time, const Texture& bitmap);

		DisplayWindow& m_window;
		std::unique_ptr<Texture> m_freezeTexture;
		std::unique_ptr<backend::ActiveBackend::native_sprite> m_freezeSprite;
		RenderStates m_freezeShader;

		bool m_rgssTransition = false;
	};
}
#endif
