#include "Graphics/Texture.h"
#include "Graphics/RenderStates.h"
#include "DisplayWindow.h"
#include "DisplayWindowSnapshot.h"

cgss::DisplayWindowSnapshot::DisplayWindowSnapshot(DisplayWindow& window) :
	m_window(window) {
}

void cgss::DisplayWindowSnapshot::init() {
	static constexpr const char* GraphicsTransitionFragmentShader = \
		"uniform float param;" \
		"uniform sampler2D texture;" \
		"uniform sampler2D transition;" \
		"const float sensibilite = 0.05;" \
		"const float scale = 1.0 + sensibilite;" \
		"void main()" \
		"{" \
		"  vec4 frag = texture2D(texture, gl_TexCoord[0].xy);" \
		"  vec4 tran = texture2D(transition, gl_TexCoord[0].xy);" \
		"  float pixel = max(max(tran.r, tran.g), tran.b);" \
		"  pixel -= (param * scale);" \
		"  if(pixel < sensibilite)" \
		"  {" \
		"	frag.a = max(0.0, sensibilite + pixel / sensibilite);" \
		"  }" \
		"  gl_FragColor = frag;" \
		"}";

	m_freezeShader.loadShaderFromMemory(GraphicsTransitionFragmentShader, sf::Shader::Type::Fragment);	
}

void cgss::DisplayWindowSnapshot::freeze(sf::RenderWindow& window) {
	if (m_freezeTexture != nullptr) {
		return;
	}
	m_window.draw();
	m_freezeTexture = std::make_unique<Texture>();
	takeSnapshotOn(window, m_freezeTexture->getTexture());
	m_freezeSprite = std::make_unique<sf::Sprite>(m_freezeTexture->getTexture());
	m_freezeSprite->setScale(1.0f / static_cast<float>(m_window.scale()), 1.0f / static_cast<float>(m_window.scale()));
}

void cgss::DisplayWindowSnapshot::takeSnapshotOn(sf::RenderWindow& window, sf::Texture& text) const {
	sf::Vector2u sc_sz = window.getSize();
	int x = 0;
	int y = 0;
	const auto screenWidth = m_window.screenWidth();
	const auto screenHeight = m_window.screenHeight();
	double sw = screenWidth * m_window.scale();
	double sh = screenHeight * m_window.scale();
	if(sc_sz.x < sw) {
		x = static_cast<int>(sw - sc_sz.x);
	} else {
		sw = static_cast<double>(sc_sz.x);
	}

	if(sc_sz.y < sh) {
		y = static_cast<int>(sh - sc_sz.y);
	} else {
		sh = static_cast<double>(sc_sz.y);
	}
	text.create(static_cast<int>(sw), static_cast<int>(sh));
	text.update(window, x, y);
}

void cgss::DisplayWindowSnapshot::transitionBasic(long time) {
	m_rgssTransition = false;
	sf::Color freeze_color(255, 255, 225, 255);
	for (long i = 1; i <= time; i++) {
		freeze_color.a = static_cast<sf::Uint8>(255 * (time - i) / time);
		m_freezeSprite->setColor(freeze_color);
		m_window.draw();
	}
}

void cgss::DisplayWindowSnapshot::transitionRGSS(long time, const Texture& bitmap) {
	m_freezeSprite->setColor(sf::Color(255, 255, 255, 255));
	m_rgssTransition = true;
	m_freezeShader.setShaderUniform("transition", bitmap.getTexture());
	for (long i = 1; i <= time; i++) {
		m_freezeShader.setShaderUniform("param", static_cast<float>(i) / time);
	}
}

static inline long normalize_long(long value, long min, long max) {
	if(value < min)
		return min;
	if(value > max)
		return max;
	return value;
}

void cgss::DisplayWindowSnapshot::transition(long time, Texture* texture) {
	if(m_freezeSprite == nullptr) {
		return;
	}

	time = normalize_long(time, 1, 0xFFFF);
	if (texture == nullptr) {
		transitionBasic(time);
	} else {
		transitionRGSS(time, *texture);
	}
	
	m_freezeSprite = nullptr;
	m_freezeTexture = nullptr;
}

std::unique_ptr<sf::Texture> cgss::DisplayWindowSnapshot::takeSnapshot(sf::RenderWindow& window) const {
	auto texture = std::make_unique<sf::Texture>();
	takeSnapshotOn(window, *texture);
	return texture;
}

void cgss::DisplayWindowSnapshot::stop() {
	/* Unfreezing Graphics */
	m_freezeSprite = nullptr;
	m_freezeTexture = nullptr;
	m_freezeShader = {};
}

void cgss::DisplayWindowSnapshot::draw(sf::RenderWindow& window) {
	// Display transition sprite
	if (m_freezeSprite != nullptr) {
		if (m_rgssTransition) {
			window.draw(*m_freezeSprite, m_freezeShader.getRenderStates());
		} else {
			window.draw(*m_freezeSprite);
		}
	}
}
