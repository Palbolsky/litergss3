#include "Viewport.h"
#include "Common/Rectangle.h"
#include "Configuration/DisplayWindowSettings.h"

cgss::Viewport::Viewport(MainEventDispatcher& eventDispatcher, const DisplayWindowSettings& windowSettings) :
	SubObserver<WindowSettingsChangeEvent>(std::bind(&Viewport::onWindowSettingsChange, this, std::placeholders::_1), eventDispatcher),
	m_screenWidth(windowSettings.video.width),
	m_screenHeight(windowSettings.video.height),
	m_smooth(windowSettings.smoothScreen) {
}

void cgss::Viewport::draw(sf::RenderTarget& target) const {
	auto* render_states = getRenderState();
	if (!m_visible) {
		return;
	}

	if (render_states) {
		const sf::Color* col;
		Rectangle* rect = getRectangle();
		// Loading Target view
		const sf::Vector2f view_size = m_view.getSize();
		sf::View tview = m_view;
		m_render->setView(tview);
		
		// Clearing render
		col = render_states->shader ? checkUpColor() : col = &sf::Color::Transparent;
		
		m_render->clear(*col);
		// Drawing sprites
		cgss::View::draw(*m_render);
		m_render->display();
		// Update internal texture & draw result to Window
		m_renderSprite->setTexture(m_render->getTexture());
		if (rect) {
			m_renderSprite->setTextureRect(rect->getRect());
			m_renderSprite->setPosition(rect->getRect().left, rect->getRect().top);
		}
		// Reset the target view
		setupView(target);
		// Draw the result
		target.draw(*m_renderSprite, *render_states);
	} else {
		target.setView(m_view);
		cgss::View::draw(target);
	}
}

void cgss::Viewport::setupView(sf::RenderTarget& target) const {
	long height = m_screenHeight;
	long width = m_screenWidth;
	sf::View defview = target.getDefaultView();
	defview.setSize(width, height);
	defview.setCenter(round(width / 2.0f), round(height / 2.0f));
	target.setView(defview);
}

void cgss::Viewport::setRenderStates(sf::RenderStates* states) {
	m_renderStates = states;
	if (states == nullptr) {
		m_renderStatesShader = nullptr;
		m_defaultRenderStates = nullptr;
		m_defaultRenderStatesShader = nullptr;
		m_colorCopy = nullptr;
	} else {
		m_renderStatesShader = const_cast<sf::Shader*>(states->shader);
	}
}

sf::RenderStates* cgss::Viewport::getRenderState() const {
	if (m_renderStates == nullptr) {
		return m_defaultRenderStates.get();
	}
	return m_renderStates;
}

sf::Shader* cgss::Viewport::getRenderStateShader() const {
	if (m_renderStatesShader == nullptr) {
		return m_defaultRenderStatesShader.get();
	}
	return m_renderStatesShader;
}

sf::Color* cgss::Viewport::checkUpColor() const {
	if (*m_color != *m_colorCopy) {
		sf::Glsl::Vec4 color(m_color->r / 255.0f, m_color->g / 255.0f, m_color->b / 255.0f, m_color->a / 255.0f);
		getRenderStateShader()->setUniform("color", color);
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
		m_render->create(m_screenWidth, m_screenHeight);
		m_render->setSmooth(m_smooth);
		m_renderSprite = std::make_unique<sf::Sprite>();
	}

	// Return if the render texture (internal_texture) is already created
	if (m_defaultRenderStates.get() != nullptr) {
		return;
	}

	// Creation of the render states
	m_defaultRenderStatesShader =
		//TODO use shaders
		//CGraphics::Get().createUniqueShader();
		nullptr;
	if (m_defaultRenderStatesShader != nullptr) {
		// Shader initialialization
		sf::Shader& shader = *m_defaultRenderStatesShader;
		if (shader.loadFromMemory(ViewportGlobalFragmentShader, sf::Shader::Fragment)) {
			shader.setUniform("texture", sf::Shader::CurrentTexture);
			//shader.setUniform("tone", __Viewport_reset_tone);
			//shader.setUniform("color", __Viewport_reset_tone);
		}
	}

	m_defaultRenderStates = std::make_unique<sf::RenderStates>(m_defaultRenderStatesShader.get());
	m_colorCopy = std::make_unique<sf::Color>(0, 0, 0, 0);
}

void cgss::Viewport::adjustDimensions(const sf::IntRect& rect) {
	auto x = rect.left;
	auto y = rect.top;
	auto width = rect.width;
	auto height = rect.height;
	adjustDimensions(x, y, width, height);
}

void cgss::Viewport::adjustZoom(float zoom) {
	m_view.zoom(zoom);
}

void cgss::Viewport::adjustAngle(float angle) {
	m_view.setRotation(-angle);
}

bool cgss::Viewport::onWindowSettingsChange(WindowSettingsChangeEvent& event) {
	m_screenWidth = event.settings.video.width;
	m_screenHeight = event.settings.video.height;
	m_smooth = event.settings.smoothScreen;
	return true;
}

void cgss::Viewport::adjustDimensions(long x, long y, long width, long height) {
	/* Adjustment for text */
	if (width & 1) {
		width++;
	}
	if (height & 1) {
		height++;
	}
	m_view.setCenter(static_cast<float>(m_ox + width / 2), static_cast<float>(m_oy + height / 2));
	m_view.setSize(static_cast<float>(width), static_cast<float>(height));
	float sw = static_cast<float>(m_screenWidth);
	float sh = static_cast<float>(m_screenHeight);
	sf::FloatRect frect(x / sw, y / sh, width / sw, height / sh);
	m_view.setViewport(frect);
}
