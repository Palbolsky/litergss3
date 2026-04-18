#ifndef CGSS_DISPLAY_WINDOW_SNAPSHOT_H
#define CGSS_DISPLAY_WINDOW_SNAPSHOT_H

#include <memory>
#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Graphics/RenderStates.h"

namespace sf {
	class RenderWindow;
	class Sprite;
	class Texture;
}

namespace cgss {
	class DisplayWindow;
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
		std::unique_ptr<sf::Texture> takeSnapshot(const sf::RenderWindow& window) const;
		void freeze(const sf::RenderWindow& window);
		void transition(long time, Texture* texture = nullptr);
		void stop();
		void draw(sf::RenderWindow& window);
		void init();
	private:
		void takeSnapshotOn(const sf::RenderWindow& window, sf::Texture& text) const;
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
