#include <cmath>
#include <cassert>
#include "LiteCGSS/Configuration/DisplayWindowSettings.h"
#include "DisplayWindow.h"
#include "LoggerViews.h"

SKA_LOGC_CONFIG(ska::LogLevel::Info, cgss::DisplayWindow)

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Debug, cgss::DisplayWindow)
#define LOG_INFO SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Info, cgss::DisplayWindow)
#define LOG_WARN SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Warn, cgss::DisplayWindow)

cgss::DisplayWindow::DisplayWindow(const DisplayWindowSettings& settings) :
	m_snapshot{ *this } {
	reload(settings, true);
}

cgss::DisplayWindow::DisplayWindow() :
	m_snapshot{ *this } {
}

cgss::DisplayWindow::~DisplayWindow() {
	stop();
}

void cgss::DisplayWindow::initUserRender() {
	m_userRenderTexture->create(m_draw.screenWidth, m_draw.screenHeight);
	m_userRenderTexture->setSmooth(m_draw.smoothScreen);

	m_userRenderSprite->setTexture(m_userRenderTexture->getTexture());
}

void cgss::DisplayWindow::reload(DisplayWindowSettings settings, bool force) {
	const auto windowCreateNeeded = 
		force ||
		settings.video.bitsPerPixel != m_settings.video.bitsPerPixel ||
		settings.fullscreen != m_settings.fullscreen;
	
	m_settings = std::move(settings);

	if (settings.headless) {
		m_gameWindow.close();
	} else {
		const auto scaledWidth = static_cast<unsigned int>(m_settings.video.width * m_settings.video.scale);
		const auto scaledHeight = static_cast<unsigned int>(m_settings.video.height * m_settings.video.scale);
		if (windowCreateNeeded) {
			const auto style = m_settings.fullscreen ? sf::Style::Fullscreen : (sf::Style::Close | sf::Style::Titlebar);
			m_gameWindow.create(sf::VideoMode { scaledWidth, scaledHeight, m_settings.video.bitsPerPixel }, m_settings.title, style, m_settings.context);
		} else {
			m_gameWindow.setSize(sf::Vector2u { scaledWidth, scaledHeight });
			m_gameWindow.setTitle(m_settings.title);
		}
		m_gameWindow.setMouseCursorVisible(m_settings.visibleMouse);
		setVsync(m_settings.vSync);
	}

	m_draw.smoothScreen = m_settings.smoothScreen;
	m_draw.scale = m_settings.video.scale;
	m_draw.frameRate = m_settings.frameRate;

	m_snapshot.init();

	const auto windowSizeChanged = force || m_draw.screenWidth != m_settings.video.width || m_draw.screenHeight != m_settings.video.height;
	if (windowSizeChanged) {
		onSizeChanged();
	}

	LOG_WARN << "Reloading window with parameters screenwidth = " << m_draw.screenWidth << " screenheight = " << m_draw.screenHeight
		<< " smoothScreen = " << m_draw.smoothScreen << " scale = " << m_draw.scale << " framerate = " << m_draw.frameRate;
}

void cgss::DisplayWindow::onSizeChanged() {
	m_draw.screenWidth = m_settings.video.width;
	m_draw.screenHeight = m_settings.video.height;

	/* Render resize */
	if (m_draw.renderTexture != nullptr) {
		m_draw.renderTexture->create(m_draw.screenWidth, m_draw.screenHeight);
	}

	/* Re-init user drawing surface */
	initUserRender();

	notifyWindowSizeChange();
}

void cgss::DisplayWindow::notifyWindowSizeChange() {
	/* This event will help by notifying subscribed viewboxes (Viewports / FramedViews / ...) */
	auto we = WindowSizeChangeEvent{};
	we.width = m_settings.video.width;
	we.height = m_settings.video.height;
	m_eventDispatcher.Observable<WindowSizeChangeEvent>::notifyObservers(we);
}

sf::RenderTarget& cgss::DisplayWindow::configureAndGetRenderTarget(sf::View& defview) {
	// Setting the default view parameters
	defview.setSize(static_cast<float>(m_draw.screenWidth), static_cast<float>(m_draw.screenHeight));
	defview.setCenter(round(m_draw.screenWidth / 2.0f), round(m_draw.screenHeight / 2.0f));
	
	// Appying the default view to the Window (used in postProcessing())
	m_gameWindow.setView(defview);
	
	// If the m_renderTexture is defined, we use it instead of the m_gameWindow (shader processing)
	if (m_draw.renderTexture != nullptr) {
		// Set the default view
		m_draw.renderTexture->setView(defview);
		// It's not cleard so we perform the clear operation
		m_draw.renderTexture->clear();
		return *m_draw.renderTexture.get();
	}
	return m_gameWindow;
}

const cgss::DisplayWindowContextSettings& cgss::DisplayWindow::getContextSettings() const {
	return m_gameWindow.getSettings();
}

const cgss::DisplayWindowSettings& cgss::DisplayWindow::getSettings() const {
	return m_settings;
}

bool cgss::DisplayWindow::isOpen() const {
	return m_gameWindow.isOpen();
}

void cgss::DisplayWindow::stop() {
	m_snapshot.stop();
	m_gameWindow.close();
}

void cgss::DisplayWindow::draw() {
	m_gameWindow.clear();

	sf::View defview = m_gameWindow.getDefaultView();
	auto& render_target = configureAndGetRenderTarget(defview);
	
	// Rendering C++ sprite stack
	drawables().draw(defview, render_target);

	postProcessing();

	m_gameWindow.display();
}

bool cgss::DisplayWindow::pollEvent(sf::Event& event) {
	return m_gameWindow.pollEvent(event);
}

void cgss::DisplayWindow::drawBrightness() {
	sf::Vertex	vertices[4];
	sf::Vector2u size = m_gameWindow.getSize();
	vertices[0].position = sf::Vector2f(0, 0);
	vertices[1].position = sf::Vector2f(0, static_cast<float>(size.y));
	vertices[2].position = sf::Vector2f(static_cast<float>(size.x), 0);
	vertices[3].position = sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y));
	vertices[0].color = vertices[1].color = vertices[2].color = vertices[3].color = sf::Color(0, 0, 0, 255 - m_draw.brightness);
	m_gameWindow.draw(vertices, 4, sf::PrimitiveType::TriangleStrip);
}

void cgss::DisplayWindow::postProcessing() {
	// Drawing render to window if finished
	if (m_draw.renderTexture) {
		m_draw.renderTexture->display();
		sf::Sprite sp(m_draw.renderTexture->getTexture());
		if (m_draw.renderState == nullptr) {
			m_gameWindow.draw(sp);
		} else {
			m_gameWindow.draw(sp, *m_draw.renderState);
		}
	}

	//Draw the "freeze" texture, if visible
	m_snapshot.draw(m_gameWindow);

	// Update the brightness (applied to m_gameWindow)
	if (m_draw.brightness != 255) {
		drawBrightness();
	}
}

void cgss::DisplayWindow::resizeScreen(unsigned int width, unsigned int height) {
	m_settings.video.width = width;
	m_settings.video.height = height;

	const auto scaledWidth = static_cast<unsigned int>(m_settings.video.width * m_settings.video.scale);
	const auto scaledHeight = static_cast<unsigned int>(m_settings.video.height * m_settings.video.scale);
	m_gameWindow.setSize(sf::Vector2u { scaledWidth, scaledHeight });
	
	onSizeChanged();
}

void cgss::DisplayWindow::setTitle(const char* title) {
	m_settings.title = sf::String { title };
	m_gameWindow.setTitle(m_settings.title);}

void cgss::DisplayWindow::setVsync(bool enable) {
	m_settings.vSync = enable;
	/* VSYNC choice */
	m_gameWindow.setVerticalSyncEnabled(m_settings.vSync);
	m_gameWindow.setFramerateLimit(!m_settings.vSync ? m_settings.frameRate : 0);
}

void cgss::DisplayWindow::setFrameRate(unsigned int frameRate) {
	m_settings.frameRate = frameRate;
	m_gameWindow.setFramerateLimit(!m_settings.vSync ? m_settings.frameRate : 0);
}

void cgss::DisplayWindow::setScale(double scale) {
	m_settings.video.scale = scale;

	const auto scaledWidth = static_cast<unsigned int>(m_settings.video.width * m_settings.video.scale);
	const auto scaledHeight = static_cast<unsigned int>(m_settings.video.height * m_settings.video.scale);
	m_gameWindow.setSize(sf::Vector2u { scaledWidth, scaledHeight });
}

void cgss::DisplayWindow::setIcon(const sf::Image& image) {
	m_gameWindow.setIcon(image.getSize().x, image.getSize().y, image.getPixelsPtr());
}

void cgss::DisplayWindow::setShader(sf::RenderStates* shader) {
	m_draw.renderState = shader;
	if (m_draw.renderTexture == nullptr && m_draw.renderState != nullptr) {
		m_draw.renderTexture = std::make_unique<sf::RenderTexture>();
		m_draw.renderTexture->create(m_draw.screenWidth, m_draw.screenHeight);
	}
}

std::unique_ptr<sf::Texture> cgss::DisplayWindow::takeSnapshot() const {
	return m_snapshot.takeSnapshot(m_gameWindow);
}

int cgss::DisplayWindow::getX() const {
	return m_gameWindow.getPosition().x;
}

int cgss::DisplayWindow::getY() const {
	return m_gameWindow.getPosition().y;
}

void cgss::DisplayWindow::transition(long time, Texture* texture) {
	m_snapshot.transition(time, texture);
}

void cgss::DisplayWindow::freeze() {
	m_snapshot.freeze(m_gameWindow);
}

void cgss::DisplayWindow::move(int x, int y) {
	m_gameWindow.setPosition(sf::Vector2i { x, y });
}

unsigned int cgss::DisplayWindow::DesktopWidth() {
	return sf::VideoMode::getDesktopMode().width;
}

unsigned int cgss::DisplayWindow::DesktopHeight() {
	return sf::VideoMode::getDesktopMode().height;
}
