#ifndef CGSS_DISPLAY_WINDOW_H
#define CGSS_DISPLAY_WINDOW_H

#include <memory>

#include <SFML/Graphics/Image.hpp>

#include "View.h"
#include "ViewAuthorizations.h"
#include "DisplayWindowDraw.h"
#include "DisplayWindowSnapshot.h"
#include "DisplayWindowUserData.h"
#include "FramedView.h"
#include "SnapshotCapturable.h"

#include "LiteCGSS/Events/MainEventDispatcher.h"
#include "LiteCGSS/Events/Patterns/SubObserver.h"
#include "LiteCGSS/Events/Patterns/Observable.h"

namespace cgss {

	struct DisplayWindowSettings;
	class DisplayWindow :
		public View,
		public SnapshotCapturable {

	public:
		DisplayWindow(const DisplayWindowSettings& settings);
		DisplayWindow(DisplayWindow&&) = default;
		DisplayWindow& operator=(DisplayWindow&&) = delete;
		DisplayWindow();
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
		const DisplayWindowContextSettings& getContextSettings() const;

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
			return { m_eventDispatcher, m_settings, m_userRenderSprite.get(), m_userRenderTexture.get() };
		}
		void initUserRender();
		void initRender();
		void drawBrightness();
		sf::RenderTarget& configureAndGetRenderTarget(sf::View& defview);
		void postProcessing();

		MainEventDispatcher m_eventDispatcher {};
		DisplayWindowSettings m_settings {};
		std::unique_ptr<sf::RenderWindow> m_gameWindow = nullptr;
		DisplayWindowSnapshot m_snapshot;
		DisplayWindowDrawData m_draw;

		std::unique_ptr<sf::Sprite> m_userRenderSprite;
		std::unique_ptr<sf::RenderTexture> m_userRenderTexture;
	};

	class Sprite;
	class Text;
	class Viewport;

	template <>
	struct ViewAuthorizations<DisplayWindow> : public AuthorizedDrawableSet<Sprite, Text, Viewport, FramedView> {};
}

#endif