#include <cmath>
#include "Viewport.h"
#include "LiteCGSS/Common/Rectangle.h"
#include "LiteCGSS/Configuration/DisplayWindowSettings.h"
#include "LoggerViews.h"

SKA_LOGC_CONFIG(ska::LogLevel::Info, cgss::ViewportItem);

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Debug, cgss::ViewportItem)
#define LOG_INFO SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Info, cgss::ViewportItem)
#define LOG_WARN SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Warn, cgss::ViewportItem)

std::unique_ptr<sf::Sprite> cgss::ViewportItem::RenderSprite = nullptr;
std::unique_ptr<sf::RenderTexture> cgss::ViewportItem::Render = nullptr;

cgss::ViewportItem::ViewportItem(Viewport*, MainEventDispatcher& eventDispatcher, const DisplayWindowSettings& windowSettings) :
	ViewBox(eventDispatcher, windowSettings) {
	m_smooth = windowSettings.smoothScreen;
}

const cgss::RenderStatesData* cgss::ViewportItem::getRenderStates() const {
	if (ShaderRenderable::directAccess() != nullptr) {
		return ShaderRenderable::directAccess();
	}
	return m_defaultRenderStates == nullptr ? nullptr : &m_defaultRenderStates->data();
}

cgss::RenderStatesData* cgss::ViewportItem::getRenderStates() {
	if (ShaderRenderable::directAccess() != nullptr) {
		return ShaderRenderable::directAccess();
	}
	return m_defaultRenderStates == nullptr ? nullptr : &m_defaultRenderStates->data();
}

void cgss::ViewportItem::onPostWindowSettingsChange(WindowSettingsChangeEvent& event) {
	m_smooth = event.settings.smoothScreen;
	if (Render != nullptr) {
		Render->setSmooth(m_smooth);
	}
}

void cgss::ViewportItem::draw(sf::RenderTarget& target) const {	
	sf::View originalView = sf::View(target.getView());

	const auto* renderStates = getRenderStates() == nullptr ? nullptr : &getRenderStates()->getRenderStates();
	if (renderStates) {
		// Loading Target view
		ViewBox::onRenderTarget(*Render, true);

		// Clearing render
		Render->clear(sf::Color::Transparent);

		// Drawing sprites
		drawables().drawFast(*Render);
		Render->display();

		// Reset the target view
		setupFullWindowView(target);

		// Draw the result
		target.draw(*RenderSprite, *renderStates);
	} else {
		ViewBox::onRenderTarget(target, false);
		drawables().drawFast(target);
	}

	target.setView(originalView);
}

std::unique_ptr<sf::Texture> cgss::ViewportItem::takeSnapshot() const {
	sf::RenderTexture render;
	sf::Sprite renderSprite;
	sf::View renderView = ViewBox::clone();
	render.create(renderView.getSize().x, renderView.getSize().y);
	renderView.setViewport(sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f));
	render.setView(renderView);
	
	const auto* renderStates = getRenderStates() == nullptr ? nullptr : &getRenderStates()->getRenderStates();
	if (renderStates) {
		// Clearing render
		render.clear(sf::Color::Transparent);

		// Drawing sprites
		drawables().drawFast(render);
		render.display();

		sf::Texture copy = sf::Texture(render.getTexture());
		renderSprite.setTexture(copy);
		renderSprite.setPosition(0, 0);

		// Draw the result
		render.clear(sf::Color::Transparent);
		render.draw(renderSprite, *renderStates);
		render.display();
	} else {
		render.clear(sf::Color::Transparent);
		drawables().drawFast(render);
		render.display();
	}

	return std::make_unique<sf::Texture>(render.getTexture());
}

void cgss::ViewportItem::setupFullWindowView(sf::RenderTarget& target) const {
	long height = getScreenHeight();
	long width = getScreenWidth();
	sf::View defview = target.getDefaultView();
	defview.setSize(static_cast<float>(width), static_cast<float>(height));
	defview.setCenter(round(width / 2.0f), round(height / 2.0f));
	target.setView(defview);
}

void cgss::Viewport::sortZ() {
	m_proxy->sortZ();
}

void cgss::Viewport::move(float x, float y) {
	m_proxy->updateViewport(x, y);
}

void cgss::Viewport::moveOrigin(long ox, long oy) {
	m_proxy->m_wantedOx = ox;
	m_proxy->m_wantedOy = oy;
	m_proxy->moveOrigin(ox, oy);
}

void cgss::Viewport::resize(long width, long height) {
	m_proxy->resize(width, height);
}

void cgss::Viewport::drawFast(sf::RenderTarget& target) const {
	m_proxy->drawFast(target);
}

void cgss::Viewport::draw(sf::RenderTarget& target) const {
	m_proxy->draw(target);
}

void cgss::Viewport::bindRenderStates(RenderStates* states) {
	m_proxy->ShaderRenderable::bindValue(states);
}

void cgss::ViewportItem::updateFromValue(const RenderStatesData* renderStatesData) {
	LOG_INFO << "Updating render states " << static_cast<const void*>(renderStatesData);
	if (renderStatesData != nullptr) {
		createRender();
	} else {
		Render = nullptr;
		RenderSprite = nullptr;
		m_defaultRenderStates = nullptr;
	}
}

void cgss::ViewportItem::scaleRenderSpriteToViewport() {
	if (RenderSprite != nullptr) {
		auto viewportBox = getViewportBox();
		RenderSprite->setTextureRect(viewportBox);
		RenderSprite->setPosition(viewportBox.left, viewportBox.top);
	}
}

void cgss::ViewportItem::createRender() {
	if (Render == nullptr) {
		Render = std::make_unique<sf::RenderTexture>();
		RenderSprite = std::make_unique<sf::Sprite>();
		Render->create(getScreenWidth(), getScreenHeight());
		Render->setSmooth(m_smooth);
		
		RenderSprite->setTexture(Render->getTexture());
	}

	scaleRenderSpriteToViewport();

	if (m_defaultRenderStates == nullptr) {
		m_defaultRenderStates = std::make_unique<RenderStates>();
	}
}

void cgss::ViewportItem::updateFromValue(const sf::IntRect* rectanglePtr) {
	auto rectangle = rectanglePtr == nullptr ? sf::IntRect{} : *rectanglePtr;
    ViewBox::resize(rectangle.width, rectangle.height);
	ViewBox::updateViewport(rectangle.left, rectangle.top);

	scaleRenderSpriteToViewport();
}

void cgss::Viewport::setRectangle(sf::IntRect rectangle) {
	m_proxy->GraphicsStackItem::setValue(std::move(rectangle));
}

sf::IntRect cgss::ViewportItem::getViewportBox() const {
	const auto* boundRectangle = GraphicsStackItem::getValue();
	if (boundRectangle != nullptr) {
		return *boundRectangle;
	}
	return getRectangleBox();
}

sf::IntRect cgss::Viewport::getViewportBox() const {
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

std::unique_ptr<sf::Texture> cgss::Viewport::takeSnapshot() const {
	return m_proxy->takeSnapshot();
}

void cgss::Viewport::onDispose() {
	//std::cerr << "Clearing viewport (explicit dispose)" << std::endl;
	m_proxy->View::drawables().clear();
}

cgss::Viewport::~Viewport() {
	//std::cerr << "Clearing viewport (destructor)" << std::endl;
	if (hasProxy()) {
		m_proxy->View::drawables().clear();
	}
}
