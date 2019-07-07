#include <cassert>
#include "Configuration/DisplayWindowSettings.h"
#include "DisplayWindow.h"

cgss::DisplayWindow::DisplayWindow(const DisplayWindowSettings& settings) :
	m_snapshot{ *this } {
	reload(settings);
}

cgss::DisplayWindow::~DisplayWindow() {
	if (m_gameWindow != nullptr) { stop(); }
}

void cgss::DisplayWindow::reload(const DisplayWindowSettings& settings) {
	m_settings = settings;
	sf::Uint32 style = sf::Style::Close | sf::Style::Titlebar;
	if (settings.fullscreen) {
		style = sf::Style::Fullscreen;
	}
	m_gameWindow = std::make_unique<sf::RenderWindow>(settings.video.vmode, std::move(settings.title), style);
	m_gameWindow->setMouseCursorVisible(false);
	
	/* VSYNC choice */
	m_gameWindow->setVerticalSyncEnabled(settings.vSync);
	if (!settings.vSync) {
		m_gameWindow->setFramerateLimit(settings.frameRate);
	}

	m_draw.screenWidth = settings.video.width;
	m_draw.screenHeight = settings.video.height;
	m_draw.smoothScreen = settings.smoothScreen;
	m_draw.scale = settings.video.scale;
	m_draw.frameRate = settings.frameRate;

	/* Render resize */
	if (m_draw.renderTexture != nullptr) {
		m_draw.renderTexture->create(m_draw.screenWidth, m_draw.screenHeight);
	}

	m_snapshot.init();

	auto we = WindowSettingsChangeEvent{};
	we.settings = settings;
	m_eventDispatcher.Observable<WindowSettingsChangeEvent>::notifyObservers(we);
}

sf::RenderTarget& cgss::DisplayWindow::configureAndGetRenderTarget(sf::View& defview) {
	// Setting the default view parameters
	defview.setSize(m_draw.screenWidth, m_draw.screenHeight);
	defview.setCenter(round(m_draw.screenWidth / 2.0f), round(m_draw.screenHeight / 2.0f));
	
	// Appying the default view to the Window (used in postProcessing())
	m_gameWindow->setView(defview);
	
	// If the m_renderTexture is defined, we use it instead of the m_gameWindow (shader processing)
	if (m_draw.renderTexture != nullptr) {
		// Set the default view
		m_draw.renderTexture->setView(defview);
		// It's not cleard so we perform the clear operation
		m_draw.renderTexture->clear();
		return *m_draw.renderTexture.get();
	}
	return *m_gameWindow;
}

bool cgss::DisplayWindow::isOpen() const {
	return m_gameWindow != nullptr && m_gameWindow->isOpen();
}

void cgss::DisplayWindow::stop() {
	m_snapshot.stop();

	/* Close the window */
	m_gameWindow->close();
	m_gameWindow = nullptr;
}

void cgss::DisplayWindow::draw() {
	//TODO Respect Liskov, poor guy...
	if (!isOpen()) {
		return;
	}
	m_gameWindow->clear();

	sf::View defview = m_gameWindow->getDefaultView();
	auto& render_target = configureAndGetRenderTarget(defview);
	
	// Rendering C++ sprite stack
	View::draw(defview, render_target);

	postProcessing();

	m_gameWindow->display();
}

void cgss::DisplayWindow::initRender() {
	if (m_draw.renderTexture == nullptr && m_draw.renderState != nullptr) {
		m_draw.renderTexture = std::make_unique<sf::RenderTexture>();
		m_draw.renderTexture->create(m_draw.screenWidth, m_draw.screenHeight);
	}
}

void cgss::DisplayWindow::drawBrightness() {
	sf::Vertex	vertices[4];
	sf::Vector2u size = m_gameWindow->getSize();
	vertices[0].position = sf::Vector2f(0, 0);
	vertices[1].position = sf::Vector2f(0, size.y);
	vertices[2].position = sf::Vector2f(size.x, 0);
	vertices[3].position = sf::Vector2f(size.x, size.y);
	vertices[0].color = vertices[1].color = vertices[2].color = vertices[3].color = sf::Color(0, 0, 0, 255 - m_draw.brightness);
	m_gameWindow->draw(vertices, 4, sf::PrimitiveType::TriangleStrip);
}

void cgss::DisplayWindow::postProcessing() {
	// Drawing render to window if finished
	if (m_draw.renderTexture) {
		m_draw.renderTexture->display();
		sf::Sprite sp(m_draw.renderTexture->getTexture());
		if (m_draw.renderState == nullptr) {
			m_gameWindow->draw(sp);
		} else {
			m_gameWindow->draw(sp, *m_draw.renderState);
		}
	}

	//Draw the "freeze" texture, if visible
	m_snapshot.draw(*m_gameWindow);

	// Update the brightness (applied to m_gameWindow)
	if (m_draw.brightness != 255) {
		drawBrightness();
	}
}

void cgss::DisplayWindow::resizeScreen(int width, int height) {
	//TODO : use an event
}

void cgss::DisplayWindow::setShader(sf::RenderStates* shader) {
	m_draw.renderState = shader;
	if (shader != nullptr) {
		initRender();
	}
}

std::unique_ptr<cgss::Texture> cgss::DisplayWindow::takeSnapshot() {
	return m_snapshot.takeSnapshot(*m_gameWindow);
}

void cgss::DisplayWindow::transition(long time, Texture* texture) {
	m_snapshot.transition(time, texture);
}

void cgss::DisplayWindow::freeze() {
	m_snapshot.freeze(*m_gameWindow);
}

