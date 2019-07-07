#include "ViewBox.h"

cgss::ViewBox::ViewBox(MainEventDispatcher& ed, const DisplayWindowSettings& windowSettings) : 
	SubObserver<WindowSettingsChangeEvent>(std::bind(&ViewBox::onWindowSettingsChange, this, std::placeholders::_1), ed),
	m_screenWidth(windowSettings.video.width),
	m_screenHeight(windowSettings.video.height) {
}

void cgss::ViewBox::move(long ox, long oy) {
	m_ox = x;
	m_oy = y;

	m_view.setCenter(std::roundf(static_cast<float>(m_ox + m_view.getSize().x) / 2.0f),
		std::roundf(static_cast<float>(m_oy + m_view.getSize().y) / 2.0f));

	udpateDimensions();
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

	udpateDimensions();
}

void cgss::ViewBox::onRenderTarget(sf::RenderTarget& renderTarget, bool copy) const {
	if (copy) {
		m_copy = m_view;
		renderTarget.setView(m_copy);
	} else {
		renderTarget.setView(m_view);
	}
	
}

void cgss::ViewBox::udpateDimensions() {
	const auto sw = static_cast<float>(m_screenWidth);
	const auto sh = static_cast<float>(m_screenHeight);
	m_view.setViewport(sf::FloatRect{ m_ox / sw, m_oy / sh, m_view.getSize().x / sw, m_view.getSize().y / sh });
}

bool cgss::ViewBox::onWindowSettingsChange(WindowSettingsChangeEvent& event) {
	m_screenWidth = event.settings.video.width;
	m_screenHeight = event.settings.video.height;

	udpateDimensions();
	return true;
}

long cgss::ViewBox::getOx() const {
	return m_ox;
}

long cgss::ViewBox::getOy() const {
	return m_oy;
}

void cgss::ViewBox::adjustZoom(float zoom) {
	m_view.zoom(zoom);
}

void cgss::ViewBox::adjustAngle(float angle) {
	m_view.setRotation(-angle);
}
