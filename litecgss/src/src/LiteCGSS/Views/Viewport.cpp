#include <cmath>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/Color.hpp>
#include "Viewport.h"
#include "LiteCGSS/Common/Rectangle.h"
#include "LiteCGSS/Common/Internal/SfRectConv.h"
#include "LiteCGSS/Configuration/DisplayWindowSettings.h"
#include "LiteCGSS/Graphics/Texture.h"
#include "LoggerViews.h"

SKA_LOGC_CONFIG(ska::LogLevel::Info, cgss::ViewportItem)

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Debug, cgss::ViewportItem)
#define LOG_INFO SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Info, cgss::ViewportItem)
#define LOG_WARN SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Warn, cgss::ViewportItem)

cgss::ViewportItem::ViewportItem(Viewport*, MainEventDispatcher& eventDispatcher, DisplayWindowUserData windowData, const DisplayWindowSettings& windowSettings) :
	ViewBox(eventDispatcher, windowSettings),
	m_windowData(std::move(windowData)) {
}

const cgss::RenderStatesData* cgss::ViewportItem::getRenderStates() const {
	if (ShaderRenderable::getValue() != nullptr) {
		return ShaderRenderable::getValue();
	}
	return m_defaultRenderStates == nullptr ? nullptr : &m_defaultRenderStates->data();
}

cgss::RenderStatesData* cgss::ViewportItem::getRenderStates() {
	if (ShaderRenderable::value() != nullptr) {
		return ShaderRenderable::value();
	}
	return m_defaultRenderStates == nullptr ? nullptr : &m_defaultRenderStates->data();
}

void cgss::ViewportItem::draw(cgss::RenderTarget& target) const {
	sf::View originalView = sf::View(target.raw().getView());

	const auto* renderStates = getRenderStates() == nullptr ? nullptr : &getRenderStates()->getRenderStates();
	if (renderStates) {
		// Loading Target view
		cgss::RenderTarget userRenderTarget{*m_windowData.userRenderTexture};
		ViewBox::onRenderTarget(userRenderTarget, true);

		// Prepare texture destination dimensions
		scaleRenderSpriteToViewport();

		// Clear it
		m_windowData.userRenderTexture->clear(sf::Color::Transparent);

		// Drawing sprites
		drawables().drawFast(userRenderTarget);
		m_windowData.userRenderTexture->display();

		// Reset the target view
		setupFullWindowView(target);

		// Draw the result
		target.raw().draw(*m_windowData.userRenderSprite, renderStates->raw());
	} else {
		ViewBox::onRenderTarget(target, false);
		drawables().drawFast(target);
	}

	target.raw().setView(originalView);
}

std::unique_ptr<cgss::Texture> cgss::ViewportItem::takeSnapshot() const {
	sf::RenderTexture render;
	sf::Sprite renderSprite;
	cgss::Camera renderView = ViewBox::clone();
	cgss::RenderTarget renderTarget{render};
	render.create(ViewBox::getWidth(), ViewBox::getHeight());
	renderView.raw().setViewport(sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f));
	render.setView(renderView.raw());

	const auto* renderStates = getRenderStates() == nullptr ? nullptr : &getRenderStates()->getRenderStates();
	if (renderStates) {
		// Clearing render
		render.clear(sf::Color::Transparent);

		// Drawing sprites
		drawables().drawFast(renderTarget);
		render.display();

		sf::Texture copy = sf::Texture(render.getTexture());
		renderSprite.setTexture(copy);
		renderSprite.setPosition(0, 0);

		// Draw the result
		render.clear(sf::Color::Transparent);
		render.draw(renderSprite, renderStates->raw());
		render.display();
	} else {
		render.clear(sf::Color::Transparent);
		drawables().drawFast(renderTarget);
		render.display();
	}

	return std::make_unique<cgss::Texture>(cgss::Texture::create(sf::Texture(render.getTexture())));
}

void cgss::ViewportItem::setupFullWindowView(cgss::RenderTarget& target) const {
	long height = getScreenHeight();
	long width = getScreenWidth();
	sf::View defview = target.raw().getDefaultView();
	defview.setSize(static_cast<float>(width), static_cast<float>(height));
	defview.setCenter(round(width / 2.0f), round(height / 2.0f));
	target.raw().setView(defview);
}

void cgss::Viewport::sortZ() {
	m_proxy->sortZ();
}

void cgss::Viewport::move(float x, float y) {
	m_proxy->moveViewport(x, y);
}

void cgss::Viewport::moveOrigin(long ox, long oy) {
	m_proxy->m_wantedOx = ox;
	m_proxy->m_wantedOy = oy;
	m_proxy->moveOrigin(ox, oy);
}

void cgss::Viewport::resize(long width, long height) {
	m_proxy->resize(width, height);
}

void cgss::Viewport::drawFast(cgss::RenderTarget& target) const {
	m_proxy->drawFast(target);
}

void cgss::Viewport::draw(cgss::RenderTarget& target) const {
	m_proxy->draw(target);
}

void cgss::Viewport::bindRenderStates(RenderStates* renderStates) {
	if (renderStates != nullptr) {
		renderStates->data().setShaderUniform("texture", cgss::CurrentTexture);
		m_proxy->ShaderRenderable::bindValue(&renderStates->data());
	} else {
		m_proxy->ShaderRenderable::bindValue(nullptr);
	}
}

void cgss::ViewportItem::updateFromValue(const RenderStatesData* renderStatesData) {
	if (renderStatesData != nullptr) {
		m_defaultRenderStates = std::make_unique<RenderStates>();
	} else {
		m_defaultRenderStates = nullptr;
	}
}

void cgss::ViewportItem::scaleRenderSpriteToViewport() const {
	assert (m_windowData.userRenderSprite != nullptr);
	const auto viewportBox = getViewportBox();
	m_windowData.userRenderSprite->setTextureRect(cgss::internal::toSf(viewportBox));
	m_windowData.userRenderSprite->setPosition(viewportBox.left, viewportBox.top);
}

void cgss::ViewportItem::updateFromValue(const cgss::IntRect* rectanglePtr) {
	auto rectangle = rectanglePtr == nullptr ? getRectangleBox() : *rectanglePtr;

	LOG_INFO << "View " << static_cast<void*>(this) << " updated with rect x = " << rectangle.left << " y = " << rectangle.top << " width = " << rectangle.width << " height = " << rectangle.height;
	LOG_DEBUG << "Rectangle is " << static_cast<const void*>(rectanglePtr);
    ViewBox::resize(rectangle.width, rectangle.height);
	ViewBox::moveViewport(rectangle.left, rectangle.top);
}

void cgss::Viewport::setRectangle(cgss::IntRect rectangle) {
	m_proxy->GraphicsStackItem::setValue(std::move(rectangle));
}

cgss::IntRect cgss::ViewportItem::getViewportBox() const {
	const auto* boundRectangle = GraphicsStackItem::getValue();
	if (boundRectangle != nullptr) {
		return *boundRectangle;
	}
	return getRectangleBox();
}

cgss::IntRect cgss::Viewport::getViewportBox() const {
	return m_proxy->getViewportBox();
}

long cgss::Viewport::getOx() const {
	return m_proxy->m_wantedOx;
}

long cgss::Viewport::getOy() const {
	return m_proxy->m_wantedOy;
}

void cgss::Viewport::setAngle(float angle) {
	m_proxy->setAngle(angle);
}

void cgss::Viewport::setZoom(float zoom) {
	m_proxy->setZoom(zoom);
}

std::unique_ptr<cgss::Texture> cgss::Viewport::takeSnapshot() const {
	return m_proxy->takeSnapshot();
}

void cgss::Viewport::onDispose() {
	m_proxy->View::drawables().clear();
}

cgss::Viewport::~Viewport() {
	if (hasProxy()) {
		m_proxy->View::drawables().clear();
	}
}
