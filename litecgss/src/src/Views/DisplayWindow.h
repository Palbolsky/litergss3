#ifndef CGSS_DISPLAY_WINDOW_H
#define CGSS_DISPLAY_WINDOW_H

#include <memory>

#include "View.h"
#include "ViewAuthorizations.h"
#include "DisplayWindowDraw.h"
#include "DisplayWindowSnapshot.h"
#include "FramedView.h"

#include "Events/MainEventDispatcher.h"
#include "Events/Patterns/SubObserver.h"

namespace cgss {

	struct DisplayWindowSettings;
	class DisplayWindow :
		public View {

	public:
		DisplayWindow(MainEventDispatcher& eventDispatcher, const DisplayWindowSettings& settings);
		virtual ~DisplayWindow();

		void setBrightness(unsigned char brightness) { m_draw.brightness = brightness; }
		unsigned char brightness() const { return m_draw.brightness; }
		long screenWidth() const { return m_draw.screenWidth; }
		long screenHeight() const { return m_draw.screenHeight; }
		bool smoothScreen() const { return m_draw.smoothScreen; }
		double scale() const { return m_draw.scale; }
		auto frameRate() const { return m_draw.frameRate; }

		void stop();
		bool isOpen() const;
		void draw();
		void update();

		std::unique_ptr<cgss::Texture> takeSnapshot();
		void freeze();
		void transition(long time, Texture* texture = nullptr);
		void resizeScreen(int width, int height);
		void setShader(sf::RenderStates* shader);

		void reload(const DisplayWindowSettings& settings);
	
		template <class View, class ... Args>
		View addView(Args&& ... args) {
			return View::create(*this, m_eventDispatcher, m_settings, std::forward<Args>(args)...);
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