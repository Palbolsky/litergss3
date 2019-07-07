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
		virtual ~ViewBox() = default;

		void move(long ox, long oy);
		void resize(long width, long height);

		ViewBox(MainEventDispatcher& ed, const DisplayWindowSettings& windowSettings);

		long getOx() const;
		long getOy() const;
		void adjustZoom(float zoom);
		void adjustAngle(float angle);

	protected:
		long getScreenWidth() const { return m_screenWidth; }
		long getScreenHeight() const { return m_screenHeight; }

		void onRenderTarget(sf::RenderTarget& renderTarget, bool copy = false) const;

		virtual void onPostWindowSettingsChange(WindowSettingsChangeEvent& event) {}

	private:
		bool onWindowSettingsChange(WindowSettingsChangeEvent& event);
		void udpateDimensions();

		sf::View m_view;
		mutable sf::View m_copy;

		long m_ox = 0;
		long m_oy = 0;
		long m_screenWidth = 640;
		long m_screenHeight = 320;
	};
}
#endif
