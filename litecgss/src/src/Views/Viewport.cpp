#include <cmath>
#include "Viewport.h"
#include "Common/Rectangle.h"
#include "Configuration/DisplayWindowSettings.h"

cgss::Viewport::Viewport(MainEventDispatcher& eventDispatcher, const DisplayWindowSettings& windowSettings) :
	ViewBox(eventDispatcher, windowSettings),
	m_smooth(windowSettings.smoothScreen) {
}

void cgss::Viewport::draw(sf::RenderTarget& target) const {
	auto& renderStates = getRenderState();
	if (!m_render) {
		return;
	}

	const sf::Color* col;

	// Loading Target view
	ViewBox::onRenderTarget(*m_render, true);
		
	// Clearing render
	col = renderStates.shader ? checkUpColor() : col = &sf::Color::Transparent;
		
	m_render->clear(*col);

	// Drawing sprites
	View::draw(*m_render);
	m_render->display();
	m_renderSprite->setTexture(m_render->getTexture());

	// Reset the target view
	setupView(target);

	// Draw the result
	target.draw(*m_renderSprite, renderStates);
}

void cgss::Viewport::setupView(sf::RenderTarget& target) const {
	long height = getScreenHeight();
	long width = getScreenWidth();
	sf::View defview = target.getDefaultView();
	defview.setSize(static_cast<float>(width), static_cast<float>(height));
	defview.setCenter(round(width / 2.0f), round(height / 2.0f));
	target.setView(defview);
}

void cgss::Viewport::setRenderStates(sf::RenderStates states) {
	m_renderStates = std::move(states);
	m_renderStatesShader = const_cast<sf::Shader*>(m_renderStates.value().shader);	
}

const sf::RenderStates& cgss::Viewport::getRenderState() const {
	return m_renderStates.value_or(m_defaultRenderStates);
}

sf::Shader& cgss::Viewport::getRenderStateShader() const {
	return m_renderStatesShader == nullptr ? m_defaultRenderStatesShader : *m_renderStatesShader;
}

sf::Color* cgss::Viewport::checkUpColor() const {
	if (*m_color != *m_colorCopy) {
		sf::Glsl::Vec4 color(m_color->r / 255.0f, m_color->g / 255.0f, m_color->b / 255.0f, m_color->a / 255.0f);
		getRenderStateShader().setUniform("color", color);
		m_colorCopy->r = m_color->r;
		m_colorCopy->g = m_color->g;
		m_colorCopy->b = m_color->b;
		m_colorCopy->a = m_color->a;
	}
	return m_color.get();
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


void cgss::Viewport::createRender() {
	// If the global viewport render doesn't exist, we create it
	if (!m_render) {
		m_render = std::make_unique<sf::RenderTexture>();
		m_render->create(getScreenWidth(), getScreenHeight());
		m_render->setSmooth(m_smooth);
		m_renderSprite = std::make_unique<sf::Sprite>();


		//TODO test if we can create shaders
		// Shader initialialization
		if (m_defaultRenderStatesShader.loadFromMemory(ViewportGlobalFragmentShader, sf::Shader::Fragment)) {
			m_defaultRenderStatesShader.setUniform("texture", sf::Shader::CurrentTexture);
			//shader.setUniform("tone", __Viewport_reset_tone);
			//shader.setUniform("color", __Viewport_reset_tone);
		}


		m_defaultRenderStates = sf::RenderStates(&m_defaultRenderStatesShader);
		m_color = std::make_unique<sf::Color>(0, 0, 0, 0);
		m_colorCopy = std::make_unique<sf::Color>(0, 0, 0, 0);
	}
}

void cgss::Viewport::onPostWindowSettingsChange(WindowSettingsChangeEvent& event) {
	m_smooth = event.settings.smoothScreen;
}
