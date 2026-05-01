// SFML implementation of cgss::DisplayWindowSnapshot — selected by CMake
// when CGSS_BACKEND=sfml (folder-based per-backend specialization). Under
// other backends, the StubDisplayWindowSnapshot template in
// `Views/DisplayWindowSnapshot.h` takes its place via std::conditional_t,
// so this TU is not compiled and these methods don't exist in the engine
// .a — the snapshot/freeze/transition feature simply isn't available.
//
// The snapshot mechanism is fundamentally backend-shaped (sf::Texture's
// "update from RenderWindow" API has no portable equivalent — raylib
// uses LoadImageFromScreen, which is a totally different model).

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Color.hpp>
#include "LiteCGSS/Backend/ActiveBackend.h"
#include "LiteCGSS/Graphics/Texture.h"
#include "LiteCGSS/Graphics/RenderStates.h"
#include "LiteCGSS/Common/NormalizeNumbers.h"
#include "LiteCGSS/Views/DisplayWindow.h"
#include "LiteCGSS/Views/DisplayWindowSnapshot.h"

// SFML uses the real DisplayWindowSnapshot (has_snapshot = true), so the
// StubDisplayWindowSnapshot template — which delegates takeSnapshot to
// detail::backendCaptureScreen — is never instantiated for the active
// DisplayWindow. We still define the hook with a stub body to keep the
// symbol linkable in case any TU instantiates the stub (tests, future
// backends compiled side-by-side).
std::unique_ptr<cgss::Texture> cgss::detail::backendCaptureScreen() {
	return nullptr;
}

cgss::DisplayWindowSnapshot::DisplayWindowSnapshot(DisplayWindow& window) :
	m_window(window) {
}

cgss::DisplayWindowSnapshot::~DisplayWindowSnapshot() = default;

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
	if (m_rgssTransition) {
		m_freezeShader.data().loadShaderFromMemory(GraphicsTransitionFragmentShader, cgss::ShaderType::Fragment);
	}
}

template <class W>
void cgss::DisplayWindowSnapshot::freeze(const W& window) {
	if (m_freezeTexture != nullptr) {
		return;
	}
	m_window.draw();
	m_freezeTexture = std::make_unique<Texture>();
	takeSnapshotOn(window, cgss::backend::native_of(*m_freezeTexture));
	m_freezeSprite = std::make_unique<sf::Sprite>(cgss::backend::native_of(*m_freezeTexture));
	m_freezeSprite->setScale(1.0f / static_cast<float>(m_window.scale()), 1.0f / static_cast<float>(m_window.scale()));
}

template <class W>
void cgss::DisplayWindowSnapshot::takeSnapshotOn(const W& window, sf::Texture& text) const {
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
	m_freezeShader.data().setShaderUniform("transition", bitmap);
	for (long i = 1; i <= time; i++) {
		m_freezeShader.data().setShaderUniform("param", static_cast<float>(i) / time);
	}
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

template <class W>
std::unique_ptr<cgss::Texture> cgss::DisplayWindowSnapshot::takeSnapshot(const W& window) const {
	sf::Texture raw;
	takeSnapshotOn(window, raw);
	return std::make_unique<cgss::Texture>(cgss::Texture::create_from_native(std::move(raw)));
}

void cgss::DisplayWindowSnapshot::stop() {
	/* Unfreezing Graphics */
	m_freezeSprite = nullptr;
	m_freezeTexture = nullptr;
	m_freezeShader = {};
}

template <class W>
void cgss::DisplayWindowSnapshot::draw(W& window) {
	// Display transition sprite
	if (m_freezeSprite != nullptr) {
		if (m_rgssTransition) {
			window.draw(*m_freezeSprite, cgss::backend::native_of(m_freezeShader.data().getRenderStates()));
		} else {
			window.draw(*m_freezeSprite);
		}
	}
}

// Explicit instantiation for the SFML backend's native window type. Other
// backends instantiate StubDisplayWindowSnapshot instead (selected via
// std::conditional_t in basic_DisplayWindow<B>).
template std::unique_ptr<cgss::Texture>
    cgss::DisplayWindowSnapshot::takeSnapshot<sf::RenderWindow>(const sf::RenderWindow&) const;
template void cgss::DisplayWindowSnapshot::freeze<sf::RenderWindow>(const sf::RenderWindow&);
template void cgss::DisplayWindowSnapshot::draw<sf::RenderWindow>(sf::RenderWindow&);
