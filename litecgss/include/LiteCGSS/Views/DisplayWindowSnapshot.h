#ifndef CGSS_DISPLAY_WINDOW_SNAPSHOT_H
#define CGSS_DISPLAY_WINDOW_SNAPSHOT_H

#include <memory>
#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Graphics/RenderStates.h"
#include "LiteCGSS/Views/DisplayWindowFwd.h"

namespace sf {
	class Sprite;
	class Texture;
}

namespace cgss {

	// No-op snapshot for backends that don't yet have an snapshot impl
	// (currently raylib). Methods accept any window type via templates so
	// the same call sites in basic_DisplayWindow<B> compile under either
	// backend; under non-SFML the calls just no-op.
	template <class DisplayWindowT>
	class StubDisplayWindowSnapshot {
	public:
		StubDisplayWindowSnapshot(DisplayWindowT&) {}
		void init() {}
		void stop() {}
		template <class W> std::unique_ptr<cgss::Texture> takeSnapshot(const W&) const { return nullptr; }
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
		template <class W> void takeSnapshotOn(const W& window, sf::Texture& text) const;
		void transitionBasic(long time);
		void transitionRGSS(long time, const Texture& bitmap);

		DisplayWindow& m_window;
		std::unique_ptr<Texture> m_freezeTexture;
		std::unique_ptr<sf::Sprite> m_freezeSprite;
		RenderStates m_freezeShader;

		bool m_rgssTransition = false;
	};
}
#endif
