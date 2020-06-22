#ifndef CGSS_DISPLAY_WINDOW_H
#define CGSS_DISPLAY_WINDOW_H

#include <memory>

#include <SFML/Graphics/Image.hpp>

#include "View.h"
#include "ViewAuthorizations.h"
#include "DisplayWindowDraw.h"
#include "DisplayWindowSnapshot.h"
#include "FramedView.h"
#include "SnapshotCapturable.h"

#include "Events/MainEventDispatcher.h"
#include "Events/Patterns/SubObserver.h"

namespace cgss {

	struct DisplayWindowSettings;
	class DisplayWindow :
		public View,
		public SnapshotCapturable {

	public:
		DisplayWindow(MainEventDispatcher& eventDispatcher, const DisplayWindowSettings& settings);
		DisplayWindow(MainEventDispatcher& eventDispatcher);
		virtual ~DisplayWindow();

		void setBrightness(unsigned char brightness) { m_draw.brightness = brightness; }
		unsigned char brightness() const { return m_draw.brightness; }
		long screenWidth() const { return m_draw.screenWidth; }
		long screenHeight() const { return m_draw.screenHeight; }
		bool smoothScreen() const { return m_draw.smoothScreen; }
		double scale() const { return m_draw.scale; }
		auto frameRate() const { return m_draw.frameRate; }
		void setIcon(const sf::Image& image);

		void stop();
		bool isOpen() const;
		void draw();
		bool pollEvent(sf::Event& event);

		std::unique_ptr<sf::Texture> takeSnapshot() const override;
		void freeze();
		void transition(long time, Texture* texture = nullptr);
		void resizeScreen(unsigned int width, unsigned int height);
		void setShader(sf::RenderStates* shader);

		void reload(DisplayWindowSettings settings);
	
		template <class View, class ViewOwner, class ... Args>
		View addView(ViewOwner& owner, Args&& ... args) {
			return View::create(owner, m_eventDispatcher, m_settings, std::forward<Args>(args)...);
		}

	private:
		void initRender();
		void drawBrightness();
		sf::RenderTarget& configureAndGetRenderTarget(sf::View& defview);
		void postProcessing();

		DisplayWindowSettings m_settings;
		MainEventDispatcher& m_eventDispatcher;
		std::unique_ptr<sf::RenderWindow> m_gameWindow = nullptr;
		DisplayWindowSnapshot m_snapshot;
		DisplayWindowDrawData m_draw;
	};

	class Sprite;
	class Text;
	class Viewport;

	template <>
	struct ViewAuthorizations<DisplayWindow> : public AuthorizedDrawableSet<Sprite, Text, Viewport, FramedView> {};
}

#endif