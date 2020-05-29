#include <cmath>
#include "Viewport.h"
#include "Common/Rectangle.h"
#include "Configuration/DisplayWindowSettings.h"

cgss::ViewportItem::ViewportItem(Viewport* owner, MainEventDispatcher& eventDispatcher, const DisplayWindowSettings& windowSettings) :
	ViewBox(eventDispatcher, windowSettings),
	m_color(0, 0, 0, 0) {
	m_smooth = windowSettings.smoothScreen;
	createRender();
}

const sf::RenderStates& cgss::ViewportItem::getRenderState() const {
	return m_renderStates.has_value() ? m_renderStates.value() : m_defaultRenderStates;
}

void cgss::ViewportItem::onPostWindowSettingsChange(WindowSettingsChangeEvent& event) {
	m_smooth = event.settings.smoothScreen;
	m_render.setSmooth(m_smooth);
}

void cgss::ViewportItem::draw(sf::RenderTarget& target) const {
	auto& renderStates = getRenderState();

	// Loading Target view
	ViewBox::onRenderTarget(m_render, true);
		
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

cgss::Viewport::Viewport() {
}

void cgss::Viewport::moveOrigin(long ox, long oy) {
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

void cgss::Viewport::setRenderStates(sf::RenderStates states) {
	m_proxy->m_renderStates = std::move(states);
	m_proxy->m_renderStatesShader = const_cast<sf::Shader*>(m_proxy->m_renderStates.value().shader);
}

sf::Shader& cgss::ViewportItem::getRenderStateShader() const {
	return m_renderStatesShader == nullptr ? m_defaultRenderStatesShader : *m_renderStatesShader;
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

	//TODO test if we can create shaders
	// Shader initialialization
	if (m_defaultRenderStatesShader.loadFromMemory(ViewportGlobalFragmentShader, sf::Shader::Fragment)) {
		m_defaultRenderStatesShader.setUniform("texture", sf::Shader::CurrentTexture);
		//shader.setUniform("tone", __Viewport_reset_tone);
		//shader.setUniform("color", __Viewport_reset_tone);
	}

	m_defaultRenderStates = sf::RenderStates(&m_defaultRenderStatesShader);
	m_renderSprite.setTexture(m_render.getTexture());
	sf::Glsl::Vec4 color(m_color.r / 255.0f, m_color.g / 255.0f, m_color.b / 255.0f, m_color.a / 255.0f);
	getRenderStateShader().setUniform("color", color);
}

void cgss::ViewportItem::updateFromRectangle(const sf::IntRect& rectangle) {
    ViewBox::resize(rectangle.width, rectangle.height);
	ViewBox::updateViewport(rectangle.left, rectangle.top);
}

void cgss::Viewport::setRectangle(sf::IntRect rectangle) {
	auto* boundRectangle = m_proxy->getBoundRectangle();
	if (boundRectangle != nullptr) {
		boundRectangle->setRect(std::move(rectangle));
	} else {
		updateFromRectangle(rectangle);
	}
}

sf::IntRect cgss::Viewport::getRectangle() const {
	const auto* boundRectangle = m_proxy->getBoundRectangle();
	if (boundRectangle != nullptr) {
		return boundRectangle->getRect();
	}
	return ViewBox::getRectangleBox();
}

long cgss::Viewport::getOx() const {
	return m_proxy->getOx();
}

long cgss::Viewport::getOy() const {
	return m_proxy->getOy();
}

void cgss::Viewport::setAngle(float angle) {
	m_proxy->setAngle(angle);
}

void cgss::Viewport::setScale(float zoom) {
	m_proxy->setScale(zoom);
}
