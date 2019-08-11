#include "FramedView.h"
#include "Viewport.h"
#include "Common/Rectangle.h"

cgss::FramedView::FramedView(MainEventDispatcher& eventDispatcher, const DisplayWindowSettings& windowSettings) :
	ViewBox(eventDispatcher, windowSettings) {
}

void cgss::FramedView::draw(sf::RenderTarget& target) const {
	drawFast(target);
}

void cgss::FramedView::drawFast(sf::RenderTarget& target) const {
	/* Draw window */
	for (unsigned long i = 0; i < m_numVerticesLine; i++) {
		target.draw(m_vertices[i], m_texture == nullptr ? sf::RenderStates::Default : sf::RenderStates{ m_texture });
	}

	/* draw internal */
	auto targetView = m_view;
	auto originalView = sf::View{ target.getView() };
	drawCalculateView(target, targetView);
	target.setView(targetView);

	View::draw(target);

	/* Draw cursor & reset view */
	target.draw(m_cursor);
	target.setView(originalView);

	/* Draw pause */
	target.draw(m_pause);
}

void cgss::FramedView::drawCalculateView(sf::RenderTarget& target, sf::View& targetView) const {
	const sf::FloatRect originViewport = target.getView().getViewport();
	sf::FloatRect targetViewport = sf::FloatRect(targetView.getViewport());
	sf::Vector2f targetCenter = sf::Vector2f(targetView.getCenter());
	sf::Vector2f targetSize = sf::Vector2f(targetView.getSize());

	// Zoom adjustment
	float zoom = m_zoom;
	targetViewport.left /= zoom;
	targetViewport.top /= zoom;
	targetViewport.width /= zoom;
	targetViewport.height /= zoom;

	// ---
	targetViewport.left += originViewport.left;
	targetViewport.top += originViewport.top;
	float dx = targetViewport.left + targetViewport.width - (originViewport.left + originViewport.width);
	if (dx > 0) {
		targetCenter.x -= targetSize.x / 2;
		targetSize.x *= ((targetViewport.width - dx) / targetViewport.width);
		targetViewport.width -= dx;
		targetCenter.x += targetSize.x / 2;
	}
	dx = originViewport.left - targetViewport.left;
	if (dx > 0) {
		targetViewport.left += dx;
	}
	float dy = targetViewport.top + targetViewport.height - (originViewport.top + originViewport.height);
	if (dy > 0) {
		targetCenter.y -= targetSize.y / 2;
		targetSize.y *= ((targetViewport.height - dy) / targetViewport.height);
		targetViewport.height -= dy;
		targetCenter.y += targetSize.y / 2;
	}
	dy = originViewport.top - targetViewport.top;
	if (dy > 0) {
		targetViewport.top += dy;
	}
	targetView.setSize(targetSize);
	targetView.setCenter(targetCenter);
	targetView.setViewport(targetViewport);
}

void cgss::FramedView::updateVertices() {
	// Checkup part
	if (m_locked || m_width <= 0 || m_height <= 0) {
		return;
	}

	if (m_stretch) {
		updateVerticesStretch(m_width, m_height);
	} else {
		updateVerticesBlt(m_width, m_height);
	}

	updateBackOpacity();
	resetPausePosition();
	updateContents();
}

void cgss::FramedView::update() {
	updatePauseSprite();
	updateCursorSprite();
	m_counter += 1;
	if (m_counter >= 128) {
		m_counter = 0;
	}
}

void cgss::FramedView::updateVerticesBlt(long wt, long ht) {
	sf::Vector2i a(0, 0);
	sf::IntRect rect;

	if (m_texture == nullptr) {
		return;
	}

	long x = m_x;
	long y = m_y;
	long xm = m_windowBuilder[0];
	long ym = m_windowBuilder[1];
	long wm = m_windowBuilder[2];;
	long hm = m_windowBuilder[3];
	long ws = m_texture->getSize().x;
	long hs = m_texture->getSize().y;

	// 1 2 3
	// 4 5 6
	// 7 8 9
	long w3 = ws - xm - wm; // <=> w6 & w9
	long w1 = xm; // <=> w4 & w7
	long delta_w = wt - w1 - w3; // <=> w2 w5 & w8 before the remaining adjustment
	// If delta_w < 0, the middle of the window doesn't exist, we try to fix the overlap of 1 on 3
	if (delta_w < 0) {
		delta_w /= 2;
		w1 += delta_w;
		w3 += delta_w;
		delta_w = wt - w1 - w3;
	}
	// Number of tiles on the part 2 of the window
	long nb2 = delta_w / wm;
	// Ajustment of the remaining pixels on the part 2 of the window
	delta_w = delta_w - (nb2 * wm);

	long h1 = ym; // <=> h2 & h3
	long h7 = hs - hm - ym; // <=> h8 & h9
	long delta_h = ht - h1 - h7; // <=> h4, h5 & h6 before the adjustment
	// If delta_h < 0, the middle of the window doesn't exist, we try to fix the overlap of 1 on 7
	if (delta_h < 0) {
		delta_h /= 2;
		h1 += delta_h;
		h7 += delta_h;
		delta_h = ht - h1 - h7;
	}
	// Number of tiles on the part 4 of the window
	long nb4 = delta_h / hm;
	// Adjustment of the remaining pixels on the part 4 of the window
	delta_h = delta_h - (nb4 * hm);

	// Allocate the vertices to perform the job
	allocateVerticesBlt(delta_w, nb2, delta_h, nb4);

	// 4 corner vertice calculation
	// [ 1, ..., ...] / [ ..., ..., ...] / [ ..., ..., ...]
	rectSet(rect, 0, 0, w1, h1);
	calculateVertices(x, y, 0, 0, a, rect);
	// [ ..., ..., 3] / [ ..., ..., ...] / [ ..., ..., ...]
	a.x = wt - w3;
	rectSet(rect, ws - w3, 0, w3, h1);
	calculateVertices(x, y, 0, -1, a, rect);
	// [ ..., ..., ...] / [ ..., ..., ...] / [ ..., ..., 9]
	a.y = ht - h7;
	rectSet(rect, ws - w3, hs - h7, w3, h7);
	calculateVertices(x, y, m_numVerticesLine - 1, -1, a, rect);
	// [ ..., ..., ...] / [ ..., ..., ...] / [ 7, ..., ...]
	a.x = 0;
	rectSet(rect, 0, hs - h7, w1, h7);
	calculateVertices(x, y, m_numVerticesLine - 1, 0, a, rect);
	// Side vertice calculation
	int count;
	// [ ..., 2, ...] / [ ..., ..., ...] / [ ..., ..., ...]
	a.x = w1;
	a.y = 0;
	rectSet(rect, xm, 0, wm, h1);
	for (count = nb2; count > 0; count--) {
		calculateVertices(x, y, 0, count, a, rect);
		a.x += wm;
	}
	if (delta_w > 0) {
		rect.width = delta_w;
		calculateVertices(x, y, 0, -2, a, rect);
	}
	// [ ..., ..., ...] / [ 4, ..., ...] / [ ..., ..., ...]
	a.x = 0;
	a.y = h1;
	rectSet(rect, 0, ym, w1, hm);
	for (count = nb4; count > 0; count--) {
		calculateVertices(x, y, count, 0, a, rect);
		a.y += hm;
	}
	if (delta_h > 0) {
		rect.height = delta_h;
		calculateVertices(x, y, nb4 + 1, 0, a, rect);
	}
	// [ ..., ..., ...] / [ ..., ..., 6] / [ ..., ..., ...]
	a.x = wt - w3;
	a.y = h1;
	rectSet(rect, ws - w3, ym, w3, hm);
	for (count = nb4; count > 0; count--) {
		calculateVertices(x, y, count, -1, a, rect);
		a.y += hm;
	}
	if (delta_h > 0) {
		rect.height = delta_h;
		calculateVertices(x, y, nb4 + 1, -1, a, rect);
	}
	// [ ..., ..., ...] / [ ..., ..., ...] / [ ..., 8, ...]
	a.x = w1;
	a.y = ht - h7;
	rectSet(rect, xm, hs - h7, wm, h7);
	for (count = nb2; count > 0; count--)
	{
		calculateVertices(x, y, m_numVerticesLine - 1, count, a, rect);
		a.x += wm;
	}
	if (delta_w > 0) {
		rect.width = delta_w;
		calculateVertices(x, y, m_numVerticesLine - 1, -2, a, rect);
	}

	// Inside vertices calculation
	int count2;
	// [ ..., ..., ...] / [ ..., 5|m , ...] / [ ..., ..., ...]
	a.x = w1;
	a.y = h1;
	rectSet(rect, xm, ym, wm, hm);
	for (count = 1; count <= nb2; count++) {
		for (count2 = 1; count2 <= nb4; count2++) {
			calculateVertices(x, y, count2, count, a, rect);
			a.y += hm;
		}
		a.y = h1;
		a.x += wm;
	}
	a.y += hm * nb4;

	if (delta_h > 0 && delta_w > 0) {
		rectSet(rect, xm, ym, delta_w, delta_h);
		calculateVertices(x, y, m_numVerticesLine - 2, -2, a, rect);
	}

	if (delta_h > 0) {
		a.x = w1;
		rectSet(rect, xm, ym, wm, delta_h);
		for (count = nb2; count > 0; count--) {
			calculateVertices(x, y, m_numVerticesLine - 2, count, a, rect);
			a.x += wm;
		}
	}

	if (delta_w > 0) {
		a.y = h1;
		rectSet(rect, xm, ym, delta_w, hm);
		for (count = nb4; count > 0; count--)
		{
			calculateVertices(x, y, count, -2, a, rect);
			a.y += hm;
		}
	}
}

void cgss::FramedView::updateVerticesStretch(long wt, long ht) {
	sf::Vector2i a(0, 0);
	sf::Vector2i s(0, 0);
	sf::IntRect rect;
	
	if (m_texture == nullptr) {
		return;
	}
	
	long x = m_x;
	long y = m_y;
	long xm = m_windowBuilder[0];
	long ym = m_windowBuilder[1];
	long wm = m_windowBuilder[2];;
	long hm = m_windowBuilder[3];
	long ws = m_texture->getSize().x;
	long hs = m_texture->getSize().y;
	// 1 2 3
	// 4 5 6
	// 7 8 9
	long w3 = ws - xm - wm; // <=> w6 & w9
	long w1 = xm; // <=> w4 & w7
	long delta_w = wt - w1 - w3; // <=> w2 w5 & w8
								 // If delta_w < 0, the middle of the window doesn't exist, we try to fix the overlap of 1 on 3
	if (delta_w < 0) {
		delta_w /= 2;
		w1 += delta_w;
		w3 += delta_w;
		delta_w = wt - w1 - w3;
	}

	long h1 = ym; // <=> h2 & h3
	long h7 = hs - hm - ym; // <=> h8 & h9
	long delta_h = ht - h1 - h7; // <=> h4, h5 & h6
								 // If delta_h < 0, the middle of the window doesn't exist, we try to fix the overlap of 1 on 7
	if (delta_h < 0) {
		delta_h /= 2;
		h1 += delta_h;
		h7 += delta_h;
		delta_h = ht - h1 - h7;
	}

	// Allocate the vertices to perform the job
	allocateVerticesStretch();

	// 4 corner vertice calculation
	// [ 1, ..., ...] / [ ..., ..., ...] / [ ..., ..., ...]
	rectSet(rect, 0, 0, w1, h1);
	calculateVertices(x, y, 0, 0, a, rect);
	// [ ..., ..., 3] / [ ..., ..., ...] / [ ..., ..., ...]
	a.x = wt - w3;
	rectSet(rect, ws - w3, 0, w3, h1);
	calculateVertices(x, y, 0, -1, a, rect);
	// [ ..., ..., ...] / [ ..., ..., ...] / [ ..., ..., 9]
	a.y = ht - h7;
	rectSet(rect, ws - w3, hs - h7, w3, h7);
	calculateVertices(x, y, m_numVerticesLine - 1, -1, a, rect);
	// [ ..., ..., ...] / [ ..., ..., ...] / [ 7, ..., ...]
	a.x = 0;
	rectSet(rect, 0, hs - h7, w1, h7);
	calculateVertices(x, y, m_numVerticesLine - 1, 0, a, rect);

	// 4 Side vertice calculation
	// [ ..., 2, ...] / [ ..., ..., ...] / [ ..., ..., ...]
	s.x = delta_w;
	s.y = h1;
	a.x = w1;
	a.y = 0;
	rectSet(rect, xm, 0, wm, h1);
	calculateVerticesStretch(x, y, 0, 1, s, a, rect);
	// [ ..., ..., ...] / [ ..., ..., ...] / [ ..., 8, ...]
	s.y = h7;
	a.x = w1;
	a.y = ht - h7;
	rectSet(rect, xm, hs - h7, wm, h7);
	calculateVerticesStretch(x, y, m_numVerticesLine - 1, 1, s, a, rect);
	// [ ..., ..., ...] / [ 4, ..., ...] / [ ..., ..., ...]
	s.x = w1;
	s.y = delta_h;
	a.x = 0;
	a.y = h1;
	rectSet(rect, 0, ym, w1, hm);
	calculateVerticesStretch(x, y, 1, 0, s, a, rect);
	// [ ..., ..., ...] / [ ..., ..., 6] / [ ..., ..., ...]
	s.x = w3;
	a.x = wt - w3;
	a.y = h1;
	rectSet(rect, ws - w3, ym, w3, hm);
	calculateVerticesStretch(x, y, 1, 2, s, a, rect);

	// Middle vertice calculation
	s.x = delta_w;
	a.x = w1;
	a.y = h1;
	rectSet(rect, xm, ym, wm, hm);
	calculateVerticesStretch(x, y, 1, 1, s, a, rect);
}

void cgss::FramedView::rectSet(sf::IntRect& rect, long x, long y, long width, long height) {
	rect.left = x;
	rect.top = y;
	rect.width = width;
	rect.height = height;
}

void cgss::FramedView::calculateVertices(long x, long y, long line, long cell, sf::Vector2i& a, sf::IntRect& rect) {
	auto& vertArr = m_vertices[line];
	int i = cell * 6;
	if (cell < 0) {
		i += vertArr.getVertexCount();
	}
	x += a.x;
	y += a.y;
	// Texture coords
	vertArr[i].texCoords = sf::Vector2f(static_cast<float>(rect.left), static_cast<float>(rect.top));
	vertArr[i + 1].texCoords = vertArr[i + 3].texCoords = sf::Vector2f(static_cast<float>(rect.left), static_cast<float>(rect.height + rect.top));
	vertArr[i + 2].texCoords = vertArr[i + 4].texCoords = sf::Vector2f(static_cast<float>(rect.width + rect.left), static_cast<float>(rect.top));
	vertArr[i + 5].texCoords = sf::Vector2f(static_cast<float>(rect.width + rect.left), static_cast<float>(rect.height + rect.top));
	// Coordinates
	vertArr[i].position = sf::Vector2f(static_cast<float>(x), static_cast<float>(y));
	vertArr[i + 1].position = vertArr[i + 3].position = sf::Vector2f(static_cast<float>(x), static_cast<float>(y + rect.height));
	vertArr[i + 2].position = vertArr[i + 4].position = sf::Vector2f(static_cast<float>(x + rect.width), static_cast<float>(y));
	vertArr[i + 5].position = sf::Vector2f(static_cast<float>(x + rect.width), static_cast<float>(y + rect.height));
}

void cgss::FramedView::calculateVerticesStretch(long x, long y, long line, long cell, sf::Vector2i& s, sf::Vector2i& a, sf::IntRect& rect) {
	auto& vertArr = m_vertices[line];
	int i = cell * 6;
	if (cell < 0) {
		i += vertArr.getVertexCount();
	}
	x += a.x;
	y += a.y;
	// Texture coords
	vertArr[i].texCoords = sf::Vector2f(static_cast<float>(rect.left), static_cast<float>(rect.top));
	vertArr[i + 1].texCoords = vertArr[i + 3].texCoords = sf::Vector2f(static_cast<float>(rect.left), static_cast<float>(rect.height + rect.top));
	vertArr[i + 2].texCoords = vertArr[i + 4].texCoords = sf::Vector2f(static_cast<float>(rect.width + rect.left), static_cast<float>(rect.top));
	vertArr[i + 5].texCoords = sf::Vector2f(static_cast<float>(rect.width + rect.left), static_cast<float>(rect.height + rect.top));
	// Coordinates
	vertArr[i].position = sf::Vector2f(static_cast<float>(x), static_cast<float>(y));
	vertArr[i + 1].position = vertArr[i + 3].position = sf::Vector2f(static_cast<float>(x), static_cast<float>(y + s.y));
	vertArr[i + 2].position = vertArr[i + 4].position = sf::Vector2f(static_cast<float>(x + s.x), static_cast<float>(y));
	vertArr[i + 5].position = sf::Vector2f(static_cast<float>(x + s.x), static_cast<float>(y + s.y));
}

void cgss::FramedView::updatePauseSprite() {
	if (m_pause.getTexture() == nullptr) {
		return;
	}

	long v = m_counter / 16;
	sf::Vector2u size = m_pause.getTexture()->getSize();
	size.x /= 2;
	size.y /= 2;
	m_pause.setTextureRect(sf::IntRect(
		(v & 0x01) == 1 ? size.x : 0,
		(v & 0x02) == 2 ? size.y : 0,
		size.x,
		size.y
	));
	sf::Color col = m_pause.getColor();
	col.a = m_opacity;
	m_pause.setColor(col);
}

void cgss::FramedView::updateCursorSprite() {
	if (m_cursor.getTexture() == nullptr) {
		return;
	}

	long v = m_counter;
	sf::Color col = m_cursor.getColor();
	col.a = m_opacity;
	if (v > 64) {
		v -= 64;
		col.a = static_cast<sf::Uint8>((m_active ? 128 + 2 * v : 128) * col.a / 255);
	} else {
		col.a = static_cast<sf::Uint8>((m_active ? 255 - 2 * v : 128) * col.a / 255);
	}
	m_cursor.setColor(col);
}

void cgss::FramedView::allocateVerticesBlt(long delta_w, long nb2, long delta_h, long nb4) {
	// Ajustment of the real amount of cell in x
	if (delta_w > 0) {
		nb2 += 3;
	}
	else {
		nb2 += 2;
	}

	// Calculate the real amount of vertices
	nb2 *= 6;

	// Ajustment of the real amount of cell in y
	if (delta_h > 0) {
		nb4 += 3;
	}
	else {
		nb4 += 2;
	}
	// Delete the vertices if the number of line is different
	// Alloction of the vertex Arrays (if no vertices or different lines)
	if (m_vertices.empty() || nb4 != m_numVerticesLine) {
		m_vertices = std::vector<sf::VertexArray>(nb4);
	}
	// Update the number of lines
	m_numVerticesLine = nb4;
	for (long i = 0; i < nb4; i++) {
		m_vertices[i].setPrimitiveType(sf::Triangles);
		if (nb2 != m_vertices[i].getVertexCount()) {
			m_vertices[i].resize(nb2);
		}
	}
}

void cgss::FramedView::allocateVerticesStretch() {
	allocateVerticesBlt(0, 1, 0, 1);
}

void cgss::FramedView::updateContents(std::unique_ptr<sf::IntRect> cursorRect) {
	if (cursorRect != nullptr) {
		m_cursorRect = std::move(cursorRect);
	}

	if (m_cursorRect != nullptr) {
		setCursorPosition(*m_cursorRect);
	}
}

void cgss::FramedView::updateBackOpacity() {
	sf::Color col = sf::Color::White;
	col.a = m_opacity * m_backOpacity / 255;
	if (!m_vertices.empty()) {
		std::size_t num_vertices = m_vertices[0].getVertexCount();
		for (std::size_t i = 0; i < m_numVerticesLine; i++) {
			auto& vertArr = m_vertices[i];
			for (std::size_t j = 0; j < num_vertices; j++) {
				vertArr[j].color = col;
			}
		}
	}
	updatePauseSprite();
	updateCursorSprite();
}

void cgss::FramedView::updateView() {
	long offset_x = m_windowBuilder[4];
	long offset_y = m_windowBuilder[5];
	long x = m_x + offset_x;
	long y = m_y + offset_y;
	long width = m_width - 2 * offset_x;
	long height = m_height - 2 * offset_y;
	
	// Update rect
	sf::IntRect& rect = m_rect;
	rect.left = x;
	rect.top = y;
	rect.width = width;
	rect.height = height;
	
	ViewBox::moveOrigin(x, y);
	resize(width, height);	
}

void cgss::FramedView::lock() {
	m_locked = true;
}

void cgss::FramedView::unlock() {
	m_locked = false;
	updateVertices();
}

bool cgss::FramedView::isLocked() const {
	return m_locked;
}

void cgss::FramedView::resetPausePosition() {
	if (!m_pauseSkin.isLoaded()) {
		return;
	}

	float pause_x = m_pauseX.has_value() ? static_cast<float>(m_pauseX.value()) : (m_width - m_pauseSkin.getSize().x) / 2.F;
	float pause_y = m_pauseY.has_value() ? static_cast<float>(m_pauseY.value()) : (m_height - m_pauseSkin.getSize().y) / 2.F;
	pause_x += m_x;
	pause_y += m_y;
	m_pause.setPosition(sf::Vector2f(pause_x, pause_y));
}

void cgss::FramedView::setCursorPosition(sf::IntRect rect) {
	if (m_cursor.getTexture() == nullptr) {
		return;
	}
	sf::Vector2f size = static_cast<sf::Vector2f>(m_cursor.getTexture()->getSize());
	m_cursor.setScale(sf::Vector2f(rect.width / size.x, rect.height / size.y));
	m_cursor.setPosition(sf::Vector2f(static_cast<float>(rect.left), static_cast<float>(rect.top)));
}

void cgss::FramedView::updateContentsOpacity() {
	const auto opacity = static_cast<uint8_t>(m_opacity * (m_contentOpacity / 255.F));

	View::updateContentsOpacity(opacity);
}

void cgss::FramedView::setPauseSkin(sf::Texture& texture) {
	m_pause.setTexture(texture);
	resetPausePosition();
	updatePauseSprite();
}

void cgss::FramedView::pause(bool pause) {
	m_paused = pause;
	updatePauseSprite();
}

void cgss::FramedView::setPausePosition(float x, float y) {
	m_pauseX = x;
	m_pauseY = y;
	resetPausePosition();
}

bool cgss::FramedView::isPaused() const {
	return m_paused;
}

void cgss::FramedView::setCursorSkin(sf::Texture& texture) {
	m_cursor.setTexture(texture);
	updateCursorSprite();
}

void cgss::FramedView::active(bool active) {
	m_active = active;
	if (m_cursorRect != nullptr) {
		setCursorPosition(*m_cursorRect);
	}
	updateCursorSprite();
}

bool cgss::FramedView::isActive() const {
	return m_active;
}

void cgss::FramedView::onMove() {
	updateVertices();
	updateView();
}

void cgss::FramedView::stretch(bool stretch) {
	m_stretch = stretch;
	updateVertices();
}

bool cgss::FramedView::isStretched() const {
	return m_stretch;
}

void cgss::FramedView::onResize() {
	updateVertices();
	updateView();
}

void cgss::FramedView::setOpacity(uint8_t opacity) {
	m_opacity = opacity;
	updateBackOpacity();
	updateContentsOpacity();
}

uint8_t cgss::FramedView::getOpacity() const {
	return m_opacity;
}

void cgss::FramedView::setContentsOpacity(uint8_t opacity) {
	m_contentOpacity = opacity;
	updateContentsOpacity();
}

uint8_t cgss::FramedView::getContentsOpacity() const {
	return m_contentOpacity;
}

void cgss::FramedView::setSkin(sf::Texture* skin) {
	m_texture = skin;
	updateVertices();
}

sf::Texture* cgss::FramedView::getSkin() const {
	return m_texture;
}
