#include <cmath>
#include "Common/Profiling.h"
#include "LiteCGSS/Views/ViewBox.h"
#include "Common/Log.h"
#include "Backend/ActiveBackend.h"
#include "LiteCGSS/Configuration/DisplayWindowSettings.h"
#include "LiteCGSS/Common/NormalizeNumbers.h"

CGSS_LOG_COMPONENT("ViewBox", warn)


namespace { using Ops = cgss::backend::ActiveBackend::Ops; }
using cgss::backend::native_of;

cgss::ViewBox::ViewBox(MainEventDispatcher& ed, const DisplayWindowSettings& windowSettings) :
	SubObserver<WindowSizeChangeEvent>(std::bind(&ViewBox::onWindowSizeChange, this, std::placeholders::_1), ed),
	m_screenWidth(windowSettings.video.width),
	m_screenHeight(windowSettings.video.height),
	m_originX(windowSettings.originX),
	m_originY(windowSettings.originY) {
	resize(m_screenWidth, m_screenHeight);
}

void cgss::ViewBox::moveOrigin(long ox, long oy) {
	CGSS_PROFILE_ZONE("cgss::ViewBox::moveOrigin");
	m_ox = ox;
	m_oy = oy;

	updateCenter();

	CGSS_LOG_DEBUG("Center updated to ox = {} oy = {}", ox, oy);

	if (m_triggerMove) {
		m_triggerMove = false;
		onMove();
		m_triggerMove = true;
	}
}

void cgss::ViewBox::updateCenter() {
	const auto sz = Ops::view_get_size(native_of(m_view));
	Ops::view_set_center(native_of(m_view),
		std::roundf(static_cast<float>(m_ox) + (sz.x / m_zoom) / 2.0f),
		std::roundf(static_cast<float>(m_oy) + (sz.y / m_zoom) / 2.0f));
}

void cgss::ViewBox::resize(long width, long height) {
	CGSS_PROFILE_ZONE("cgss::ViewBox::resize");
	/* Adjustment for text */
	if (width & 1) {
		width++;
	}
	if (height & 1) {
		height++;
	}
	{
		const auto sz = Ops::view_get_size(native_of(m_view));
		CGSS_LOG_INFO("Size was width = {} height = {}", sz.x, sz.y);
	}
	Ops::view_set_size(native_of(m_view),
		static_cast<float>(width) * m_zoom,
		static_cast<float>(height) * m_zoom);

	/* Resize the viewport (don't move) */
	moveViewport(m_x, m_y);

	updateCenter();

	{
		const auto sz = Ops::view_get_size(native_of(m_view));
		CGSS_LOG_INFO("Resizing to width = {} height = {}", sz.x, sz.y);
	}

	if (m_triggerResize) {
		m_triggerResize = false;
		onResize();
		m_triggerResize = true;
	}
}

void cgss::ViewBox::onRenderTarget(cgss::RenderTarget& renderTarget, bool copy) const {
	if (copy) {
		m_copy = m_view;
		Ops::target_set_view(native_of(renderTarget), native_of(m_copy));
	} else {
		Ops::target_set_view(native_of(renderTarget), native_of(m_view));
	}
}

void cgss::ViewBox::moveViewport(float x, float y) {
	CGSS_PROFILE_ZONE("cgss::ViewBox::moveViewport");
	const auto sw = static_cast<float>(m_screenWidth);
	const auto sh = static_cast<float>(m_screenHeight);
	m_x = x;
	m_y = y;
	const auto sz = Ops::view_get_size(native_of(m_view));
	const auto origin = getOrigin();
	Ops::view_set_viewport(native_of(m_view),
		(m_x + origin.x) / sw, (m_y + origin.y) / sh,
		(sz.x / m_zoom) / sw, (sz.y / m_zoom) / sh);

	CGSS_LOG_DEBUG("Viewbox {} has been moved to x = {} y = {}", static_cast<void*>(this), x, y);
	auto event = ViewportChangeEvent { getRectangleBox() };
	Observable<ViewportChangeEvent>::notifyObservers(event);
}

bool cgss::ViewBox::onWindowSizeChange(WindowSizeChangeEvent& event) {
	m_screenWidth = event.width;
	m_screenHeight = event.height;
	m_originX = event.originX;
	m_originY = event.originY;

	moveViewport(m_x, m_y);

	CGSS_LOG_INFO("Window size has changed to screenwidth = {} screenheight = {}", m_screenWidth, m_screenHeight);
	return true;
}

long cgss::ViewBox::getOx() const {
	return m_ox;
}

long cgss::ViewBox::getOy() const {
	return m_oy;
}

float cgss::ViewBox::getWidth() const {
	return Ops::view_get_size(native_of(m_view)).x / m_zoom;
}

float cgss::ViewBox::getHeight() const {
	return Ops::view_get_size(native_of(m_view)).y / m_zoom;
}

void cgss::ViewBox::setZoom(float zoom) {
	m_zoom = 1.0 / cgss::normalize_double(zoom, 0.001, 1000.0);
	const auto rect = getRectangleBox();
	Ops::view_reset(native_of(m_view), rect.left, rect.top, rect.width, rect.height);
	Ops::view_zoom(native_of(m_view), m_zoom);
	updateCenter();
	Ops::view_set_rotation(native_of(m_view), m_angle);
	const auto sz = Ops::view_get_size(native_of(m_view));
	CGSS_LOG_DEBUG("Zoom of {} transformed width = {} height = {}", m_zoom, sz.x, sz.y);
}

float cgss::ViewBox::getZoom() const {
	return m_zoom;
}

void cgss::ViewBox::setAngle(float angle) {
	m_angle = -angle;
	Ops::view_set_rotation(native_of(m_view), -angle);
}

float cgss::ViewBox::getAngle() const {
	return m_angle;
}

cgss::IntRect cgss::ViewBox::getRectangleBox() const {
	const auto percentBox = Ops::view_get_viewport(native_of(m_view));
	const auto sw = static_cast<float>(m_screenWidth);
	const auto sh = static_cast<float>(m_screenHeight);
	const auto origin = getOrigin();
	return cgss::IntRect {
		static_cast<int>(std::round(percentBox.left * sw - origin.x)),
		static_cast<int>(std::round(percentBox.top * sh - origin.y)),
		static_cast<int>(percentBox.width * sw),
		static_cast<int>(percentBox.height * sh)
	};
}

void cgss::ViewBox::setScreenSpace(bool screenSpace) {
	if (m_screenSpace == screenSpace) {
		return;
	}
	m_screenSpace = screenSpace;
	moveViewport(m_x, m_y);
}

cgss::Point cgss::ViewBox::getOrigin() const {
	if (m_screenSpace) {
		return {};
	}
	return { static_cast<float>(m_originX), static_cast<float>(m_originY) };
}
