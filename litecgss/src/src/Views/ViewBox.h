#ifndef CGSS_INNER_VIEW_H
#define CGSS_INNER_VIEW_H

#include "Events/Patterns/SubObserver.h"
#include "Events/WindowSettingsChangeEvent.h"
#include "Events/MainEventDispatcher.h"

namespace cgss {

	struct DisplayWindowSettings;
	class ViewBox :
		private SubObserver<WindowSettingsChangeEvent> {

	public:
		ViewBox(ViewBox&&) noexcept = default;
		ViewBox& operator=(ViewBox&&) noexcept = default;
		ViewBox(const ViewBox&) = delete;
		ViewBox& operator=(const ViewBox&) = delete;
		
		virtual ~ViewBox() = default;

		void moveOrigin(long ox, long oy);
		void resize(long width, long height);

		ViewBox(MainEventDispatcher& ed, const DisplayWindowSettings& windowSettings);

		long getOx() const;
		long getOy() const;
		void setScale(float zoom);
		void setAngle(float angle);
		float getScale() const;

		MainEventDispatcher& getEventDispatcher() { return m_eventDispatcher; }
	protected:
		long getScreenWidth() const { return m_screenWidth; }
		long getScreenHeight() const { return m_screenHeight; }
		float getWidth() const;
		float getHeight() const;
		sf::View clone() const { return m_view; }
		void updateViewport(float x, float y);
		sf::IntRect getRectangleBox() const;

		void onRenderTarget(sf::RenderTarget& renderTarget, bool copy = false) const;

		virtual void onPostWindowSettingsChange(WindowSettingsChangeEvent& event) {}
		virtual void onMove() {}
		virtual void onResize() {}

	private:
		bool onWindowSettingsChange(WindowSettingsChangeEvent& event);
		void updateCenter();

		MainEventDispatcher& m_eventDispatcher;

		sf::View m_view;
		mutable sf::View m_copy;

		bool m_triggerMove = true;
		bool m_triggerResize = true;

		long m_ox = 0;
		long m_oy = 0;
		long m_screenWidth = 640;
		long m_screenHeight = 320;
		float m_zoom = 1.f;
	};
}
#endif
