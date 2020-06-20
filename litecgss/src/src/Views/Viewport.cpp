#include <cmath>
#include "Viewport.h"
#include "Common/Rectangle.h"
#include "Configuration/DisplayWindowSettings.h"
#include "LoggerViews.h"

SKA_LOGC_CONFIG(ska::LogLevel::Info, cgss::ViewportItem);

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Debug, cgss::ViewportItem)
#define LOG_INFO SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Info, cgss::ViewportItem)
#define LOG_WARN SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Warn, cgss::ViewportItem)

cgss::ViewportItem::ViewportItem(Viewport*, MainEventDispatcher& eventDispatcher, const DisplayWindowSettings& windowSettings) :
	ViewBox(eventDispatcher, windowSettings),
	m_color(sf::Color::Transparent) {
	m_smooth = windowSettings.smoothScreen;
	createRender();
}

const cgss::RenderStates& cgss::ViewportItem::getRenderStates() const {
	return getBoundRenderStates() != nullptr ? *getBoundRenderStates() : m_defaultRenderStates;
}

cgss::RenderStates& cgss::ViewportItem::getRenderStates() {
	return getBoundRenderStates() != nullptr ? *getBoundRenderStates() : m_defaultRenderStates;
}

void cgss::ViewportItem::onPostWindowSettingsChange(WindowSettingsChangeEvent& event) {
	m_smooth = event.settings.smoothScreen;
	m_render.setSmooth(m_smooth);
}

void cgss::ViewportItem::draw(sf::RenderTarget& target) const {	
	sf::View originalView = sf::View(target.getView());

	// Loading Target view
	ViewBox::onRenderTarget(m_render, true);
	
	const auto& renderStates = getRenderStates().getRenderStates();
	// Clearing render
	const auto& color = renderStates.shader ? m_color : sf::Color::Transparent;
	m_render.clear(color);

	// Drawing sprites
	drawables().drawFast(m_render);
	m_render.display();

	// Reset the target view
	setupFullWindowView(target);

	// Draw the result
	target.draw(m_renderSprite, renderStates);

	target.setView(originalView);
}

std::unique_ptr<sf::Texture> cgss::ViewportItem::takeSnapshot() const {
	sf::RenderTexture render;
	sf::Sprite renderSprite;
	sf::View renderView = ViewBox::clone();
	render.create(renderView.getSize().x, renderView.getSize().y);
	renderView.setViewport(sf::FloatRect(0.0f, 0.0f, 1.0f, 1.0f));
	render.setView(renderView);
	
	const auto& renderStates = getRenderStates().getRenderStates();

	// Clearing render
	const auto& color = renderStates.shader ? m_color : sf::Color::Transparent;
	render.clear(color);

	// Drawing sprites
	drawables().drawFast(render);
	render.display();

	sf::Texture copy = sf::Texture(render.getTexture());
	renderSprite.setTexture(copy);
	renderSprite.setPosition(0, 0);

	// Draw the result
	render.clear(sf::Color::Transparent);
	render.draw(renderSprite, renderStates);
	render.display();

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
	m_proxy->bindRenderStates(states);
}

/* ---- Shader Part ---- */
/* Thaks to https://github.com/Ancurio/mkxp/blob/master/shader/sprite.frag (Ancurio & urkle) */
static const std::string ViewportGlobalFragmentShader = \
"uniform sampler2D texture;" \
"uniform vec4 tone;" \
"uniform vec4 color;" \
"const vec3 lumaF = vec3(.299, .587, .114);" \
"void main()" \
"{" \
"   vec4 frag = texture2D(texture, gl_TexCoord[0].xy);" \
/*"   frag.rgb = mix(frag.rgb, gl_Color.rgb, cola);" \*/
"   frag.rgb = mix(frag.rgb, color.rgb, color.a);" \
"   float luma = dot(frag.rgb, lumaF);" \
"   frag.rgb += tone.rgb;" \
"   frag.rgb = mix(frag.rgb, vec3(luma), tone.w);" \
"   frag.a *= gl_Color.a;" \
"   gl_FragColor = frag;" \
"}";


void cgss::ViewportItem::createRender() {
	m_render.create(getScreenWidth(), getScreenHeight());
	m_render.setSmooth(m_smooth);
	m_renderSprite.setTexture(m_render.getTexture());
	auto viewportBox = getViewportBox();
	m_renderSprite.setTextureRect(viewportBox);
	m_renderSprite.setPosition(viewportBox.left, viewportBox.top);

	if (m_defaultRenderStates.loadShaderFromMemory(ViewportGlobalFragmentShader, sf::Shader::Fragment)) {
		m_defaultRenderStates.setShaderUniform("texture", sf::Shader::CurrentTexture);
		m_defaultRenderStates.setShaderUniform("tone", sf::Glsl::Vec4 {});
	}

	setColor(nullptr);
}

void cgss::ViewportItem::updateFromRectangle(const sf::IntRect& rectangle) {
    ViewBox::resize(rectangle.width, rectangle.height);
	ViewBox::updateViewport(rectangle.left, rectangle.top);

	m_renderSprite.setTextureRect(rectangle);
	m_renderSprite.setPosition(rectangle.left, rectangle.top);
}

void cgss::Viewport::setTone(const sf::Glsl::Vec4* toneValue) {
	auto& renderStates = m_proxy->getRenderStates();
	renderStates.setShaderUniform("tone", toneValue == nullptr ? sf::Glsl::Vec4 {} : *toneValue);
	LOG_DEBUG << "Setting tone " << static_cast<const void*>(toneValue);
}

void cgss::ViewportItem::setColor(const sf::Color* colorValue) {
	auto& renderStates = getRenderStates();
	m_color = colorValue == nullptr ? sf::Color {0, 0, 0, 0} : *colorValue;
	sf::Glsl::Vec4 shaderColor(m_color.r / 255.0f, m_color.g / 255.0f, m_color.b / 255.0f, m_color.a / 255.0f);
	renderStates.setShaderUniform("color", shaderColor);
	LOG_DEBUG << "Setting color " << static_cast<const void*>(colorValue);
}

void cgss::Viewport::setColor(const sf::Color* colorValue) {
	m_proxy->setColor(colorValue);
}

void cgss::Viewport::setRectangle(sf::IntRect rectangle) {
	auto* boundRectangle = m_proxy->getBoundRectangle();
	if (boundRectangle != nullptr) {
		boundRectangle->setRect(std::move(rectangle));
	} else {
		m_proxy->updateFromRectangle(rectangle);
	}
}

sf::IntRect cgss::ViewportItem::getViewportBox() const {
	const auto* boundRectangle = getBoundRectangle();
	if (boundRectangle != nullptr) {
		return boundRectangle->getRect();
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
