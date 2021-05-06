#include <cmath>
#include "ViewBox.h"
#include "LoggerViews.h"
#include "LiteCGSS/Configuration/DisplayWindowSettings.h"
#include "LiteCGSS/Common/NormalizeNumbers.h"

SKA_LOGC_CONFIG(ska::LogLevel::Warn, cgss::ViewBox)

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Debug, cgss::ViewBox)
#define LOG_INFO SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Info, cgss::ViewBox)
#define LOG_WARN SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Warn, cgss::ViewBox)

cgss::ViewBox::ViewBox(MainEventDispatcher& ed, const DisplayWindowSettings& windowSettings) : 
	SubObserver<WindowSizeChangeEvent>(std::bind(&ViewBox::onWindowSizeChange, this, std::placeholders::_1), ed),
	m_screenWidth(windowSettings.video.width),
	m_screenHeight(windowSettings.video.height) {
	resize(m_screenWidth, m_screenHeight);
}

void cgss::ViewBox::moveOrigin(long ox, long oy) {
	m_ox = ox;
	m_oy = oy;

	updateCenter();

	LOG_DEBUG << "Center updated to ox = " << ox << " oy = " << oy;

	if (m_triggerMove) {
		m_triggerMove = false;
		onMove();
		m_triggerMove = true;
	}
}

void cgss::ViewBox::updateCenter() {
	const auto& viewBox = m_view.getSize() / m_zoom;
	m_view.setCenter(std::roundf(static_cast<float>(m_ox) + viewBox.x / 2.0f),
		std::roundf(static_cast<float>(m_oy) + viewBox.y / 2.0f));
}

void cgss::ViewBox::resize(long width, long height) {
	/* Adjustment for text */
	if (width & 1) {
		width++;
	}
	if (height & 1) {
		height++;
	}
	LOG_INFO << "Size was width = " << m_view.getSize().x << " height = " << m_view.getSize().y;
	m_view.setSize(static_cast<float>(width) * m_zoom, static_cast<float>(height) * m_zoom);
	
	/* Resize the viewport (don't move) */
	const auto& percentBox = m_view.getViewport();
	const auto sw = static_cast<float>(m_screenWidth);
	const auto sh = static_cast<float>(m_screenHeight);
	moveViewport(percentBox.left * sw, percentBox.top * sh);

	updateCenter();

	LOG_INFO << "Resizing to width = " << m_view.getSize().x << " height = " << m_view.getSize().y;

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

void cgss::ViewBox::moveViewport(float x, float y) {
	const auto sw = static_cast<float>(m_screenWidth);
	const auto sh = static_cast<float>(m_screenHeight);
	m_x = x;
	m_y = y;
	const auto& viewBox = m_view.getSize() / m_zoom;
	m_view.setViewport(sf::FloatRect{ m_x / sw, m_y / sh, viewBox.x / sw, viewBox.y / sh });

	LOG_DEBUG << "Viewbox " << static_cast<void*>(this) <<  " has been moved to x = " << x << " y = " << y;
	auto event = ViewportChangeEvent { getRectangleBox() };
	Observable<ViewportChangeEvent>::notifyObservers(event);
}

bool cgss::ViewBox::onWindowSizeChange(WindowSizeChangeEvent& event) {
	m_screenWidth = event.width;
	m_screenHeight = event.height;

	moveViewport(m_x, m_y);

	LOG_INFO << "Window size has changed to screenwidth = " << m_screenWidth << " screenheight = " << m_screenHeight;
	return true;
}

long cgss::ViewBox::getOx() const {
	return m_ox;
}

long cgss::ViewBox::getOy() const {
	return m_oy;
}

float cgss::ViewBox::getWidth() const {
	return m_view.getSize().x / m_zoom;
}

float cgss::ViewBox::getHeight() const {
	return m_view.getSize().y / m_zoom;
}

void cgss::ViewBox::setZoom(float zoom) {
	m_zoom = 1.0 / cgss::normalize_double(zoom, 0.001, 1000.0);
	m_view.reset(sf::FloatRect { getRectangleBox() });
	m_view.zoom(m_zoom);
	LOG_DEBUG << "Zoom of " << m_zoom << " transformed width = " << m_view.getSize().x << " height = " << m_view.getSize().y;
}

float cgss::ViewBox::getZoom() const {
	return m_zoom;
}

void cgss::ViewBox::setAngle(float angle) {
	m_angle = -angle;
	m_view.setRotation(-angle);
}

float cgss::ViewBox::getAngle() const {
	return m_angle;
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
