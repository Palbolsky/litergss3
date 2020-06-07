#include <cmath>
#include "ViewBox.h"
#include "LoggerViews.h"

SKA_LOGC_CONFIG(ska::LogLevel::Disabled, cgss::ViewBox);

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Debug, cgss::ViewBox)
#define LOG_INFO SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Info, cgss::ViewBox)
#define LOG_WARN SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Warn, cgss::ViewBox)

cgss::ViewBox::ViewBox(MainEventDispatcher& ed, const DisplayWindowSettings& windowSettings) : 
	SubObserver<WindowSettingsChangeEvent>(std::bind(&ViewBox::onWindowSettingsChange, this, std::placeholders::_1), ed),
	m_screenWidth(windowSettings.video.width),
	m_screenHeight(windowSettings.video.height) {
	resize(m_screenWidth, m_screenHeight);
}

void cgss::ViewBox::moveOrigin(long ox, long oy) {
	m_ox = ox;
	m_oy = oy;

	updateCenter();

	LOG_INFO << "Center updated to ox = " << ox << " oy = " << oy;

	if (m_triggerMove) {
		m_triggerMove = false;
		onMove();
		m_triggerMove = true;
	}
}

void cgss::ViewBox::updateCenter() {
	m_view.setCenter(std::roundf(static_cast<float>(m_ox + m_view.getSize().x) / 2.0f),
		std::roundf(static_cast<float>(m_oy + m_view.getSize().y) / 2.0f));
}

void cgss::ViewBox::resize(long width, long height) {
	/* Adjustment for text */
	if (width & 1) {
		width++;
	}
	if (height & 1) {
		height++;
	}
	m_view.setSize(static_cast<float>(width), static_cast<float>(height));
	updateCenter();

	LOG_INFO << "Resizing to width = " << width << " height = " << height;

	if (m_triggerResize) {
		m_triggerResize = false;
		onResize();
		m_triggerResize = true;
	}
}

void cgss::ViewBox::onRenderTarget(sf::RenderTarget& renderTarget, bool copy) const {
	if (copy) {
		m_copy = m_view;
		renderTarget.setView(m_copy);
	} else {
		renderTarget.setView(m_view);
	}
}

void cgss::ViewBox::updateViewport(float x, float y) {
	const auto sw = static_cast<float>(m_screenWidth);
	const auto sh = static_cast<float>(m_screenHeight);
	m_view.setViewport(sf::FloatRect{ x / sw, y / sh, m_view.getSize().x / sw, m_view.getSize().y / sh });
}

bool cgss::ViewBox::onWindowSettingsChange(WindowSettingsChangeEvent& event) {
	m_screenWidth = event.settings.video.width;
	m_screenHeight = event.settings.video.height;

	LOG_INFO << "Window settings have changed to screenwidth = " << m_screenWidth << " screenheight = " << m_screenHeight;

	onPostWindowSettingsChange(event);
	return true;
}

long cgss::ViewBox::getOx() const {
	return m_ox;
}

long cgss::ViewBox::getOy() const {
	return m_oy;
}

float cgss::ViewBox::getWidth() const {
	return m_view.getSize().x;
}

float cgss::ViewBox::getHeight() const {
	return m_view.getSize().y;
}

void cgss::ViewBox::setScale(float zoom) {
	m_zoom = zoom;
	m_view.zoom(zoom);
}

float cgss::ViewBox::getScale() const {
	return m_zoom;
}

void cgss::ViewBox::setAngle(float angle) {
	m_view.setRotation(-angle);
}

sf::IntRect cgss::ViewBox::getRectangleBox() const {
	const auto& percentBox = m_view.getViewport();
	const auto sw = static_cast<float>(m_screenWidth);
	const auto sh = static_cast<float>(m_screenHeight);
	return sf::IntRect { 
		static_cast<int>(percentBox.left * sw),
		static_cast<int>(percentBox.top * sh),
		static_cast<int>(percentBox.width * sw),
		static_cast<int>(percentBox.height * sh)
	};
}
