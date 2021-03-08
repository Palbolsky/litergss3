#include <cstring>
#include "LoggerViews.h"
#include "FramedView.h"
#include "Viewport.h"
#include "LiteCGSS/Common/Rectangle.h"

SKA_LOGC_CONFIG(ska::LogLevel::Warn, cgss::FramedViewItem)

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Debug, cgss::FramedViewItem)
#define LOG_INFO SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Info, cgss::FramedViewItem)
#define LOG_WARN SKA_LOGC_STATIC(GetLoggerViews(), ska::LogLevel::Warn, cgss::FramedViewItem)

static void RectSet(sf::IntRect& rect, long x, long y, long width, long height) {
	rect.left = x;
	rect.top = y;
	rect.width = width;
	rect.height = height;
}

cgss::FramedViewItem::FramedViewItem(FramedView*, MainEventDispatcher& eventDispatcher, DisplayWindowUserData windowData, const DisplayWindowSettings& windowSettings, std::weak_ptr<ViewportItem> parent) :
	ViewBox(eventDispatcher, windowSettings),
	SubObserver<ViewportChangeEvent>(std::bind(&FramedViewItem::onViewportChange, this, std::placeholders::_1), *this),
	m_windowData(std::move(windowData)),
	m_parent(std::move(parent)) {
}

bool cgss::FramedViewItem::onViewportChange(cgss::ViewportChangeEvent& event) {
	if (m_bondRectViewport == nullptr) {
		return true;
	}

	auto& viewportBox = event.viewportBox;
	m_bondRectViewport->setValue(viewportBox);
	return true;
}

void cgss::FramedViewItem::bindRectangleViewport(Rectangle* output) {
	m_bondRectViewport = output;
}

void cgss::FramedViewItem::draw(sf::RenderTarget& target) const {
	drawFast(target);
}

void cgss::FramedViewItem::drawFast(sf::RenderTarget& target) const {
	/* Draw window */
	for (auto& vertice : m_vertices) {
		target.draw(vertice, m_texture);
	}

	/* draw internal */
	auto targetView = ViewBox::clone();
	auto originalView = sf::View{ target.getView() };
	auto parentView = m_parent.lock();
	if (parentView != nullptr) {
		drawCalculateView(*parentView, targetView);
	}

	if (targetView.getViewport().height > 0 && targetView.getViewport().width > 0) {
		target.setView(targetView);
		drawables().drawFast(target);
	}

	/* Draw cursor & reset view */
	target.draw(m_cursor);
	target.setView(originalView);

	if (m_paused) {
		/* Draw pause */
		target.draw(m_pause);
	}
}

void cgss::FramedViewItem::drawCalculateView(ViewportItem& parent, sf::View& targetView) const {
	// TODO : can be based on event system ? when parent viewport is moved / resized.
	LOG_DEBUG << "Draw calculate view";

	const sf::IntRect parentViewport = parent.getViewportBox();
	float zoom = parent.getZoom();

	const auto vx0 = parentViewport.width / 2.f - (parentViewport.width / 2.f) / zoom; // <- When vp.zoom = 0.5, zoom = 2
	const auto vy0 = parentViewport.height / 2.f - (parentViewport.height / 2.f) / zoom;

	sf::Vector2f targetCenter = sf::Vector2f(targetView.getCenter());
	sf::Vector2f targetSize = sf::Vector2f(targetView.getSize());
	sf::FloatRect targetViewport = sf::FloatRect(targetView.getViewport());

	const auto sw = ViewBox::getScreenWidth();
	const auto sh = ViewBox::getScreenHeight();

	// Get the real view coordinate in pixel
	targetViewport.left *= sw;
	targetViewport.top *= sh;
	targetViewport.width *= sw;
	targetViewport.height *= sh;

	// Translate the window rect in the correct space according to the viewport properties
	targetViewport.left -= parent.getOx();
	targetViewport.top -= parent.getOy();

	// Zoom adjustment
	targetViewport.left /= zoom;
	targetViewport.top /= zoom;
	targetViewport.width /= zoom;
	targetViewport.height /= zoom;

	targetViewport.left += vx0;
	targetViewport.top += vy0;

	// attempt to fix viewport coordinate
	targetViewport.left += parentViewport.left;
	targetViewport.top += parentViewport.top;

	// <-
	// Fix the top/left coordinate of the view in order to make it stay in the viewport
	float dy = parentViewport.top - targetViewport.top;
	// If dy > 0 then the window top is outside of the viewport, we need to remove dy from the height & add dy to the y
	if (dy > 0) {
		targetViewport.top += dy;
		targetSize.y -= dy * zoom;
		targetViewport.height -= dy;
		targetCenter.y = dy * zoom + targetViewport.height * zoom / 2;
	}
	float dx = parentViewport.left - targetViewport.left;
	if (dx > 0) {
		targetViewport.left += dx;
		targetSize.x -= dx * zoom;
		targetViewport.width -= dx;
		targetCenter.x = dx * zoom + targetViewport.width * zoom / 2;
	}

	// Fix the bottom/right coordinate of the view in order to make it stay in the viewport
	dy = (targetViewport.top + targetViewport.height) - (parentViewport.top + parentViewport.height);
	// If dy > 0 then the window bottom is outside of the viewport, we need to remove dy from the height and adjust the center
	if (dy > 0) {
		targetViewport.height -= dy;
		targetSize.y -= dy * zoom;
		targetCenter.y -= dy * zoom / 2;
	}
	dx = (targetViewport.left + targetViewport.width) - (parentViewport.left + parentViewport.width);
	if (dx > 0) {
		targetViewport.width -= dx;
		targetSize.x -= dx * zoom;
		targetCenter.x -= dx * zoom / 2;
	}

	LOG_DEBUG << "Calculated view with center = " << targetCenter.x << ", " << targetCenter.y;

	// Set the view
	targetView.setCenter(targetCenter);
	targetView.setSize(targetSize);
	targetView.setViewport(sf::FloatRect(targetViewport.left / sw, targetViewport.top / sh, targetViewport.width / sw, targetViewport.height / sh));
}

void cgss::FramedViewItem::updateVertices() {
	// Checkup part
	if (m_locked || m_wantedWidth <= 0 || m_wantedHeight <= 0) {
		LOG_DEBUG << "Invalid FramedViewItem state in updateVertices (" <<
			"lock = " << (m_locked ? "true" : "false") << " width = " << m_wantedWidth <<  " height = " << m_wantedHeight << ")" ;
		return;
	}

	if (m_stretch) {
		updateVerticesStretch(static_cast<long>(m_wantedWidth), static_cast<long>(m_wantedHeight));
	} else {
		updateVerticesBlt(static_cast<long>(m_wantedWidth), static_cast<long>(m_wantedHeight));
	}

	updateBackOpacity();
	resetPausePosition();
	updateContents();
}

void cgss::FramedViewItem::update() {
	updatePauseSprite();
	updateCursorSprite();
	m_counter += 1;
	if (m_counter >= 128) {
		m_counter = 0;
	}
}

void cgss::FramedViewItem::updateVerticesBlt(long wt, long ht) {
	sf::Vector2i a(0, 0);
	sf::IntRect rect;

	if (m_texture == nullptr) {
		LOG_WARN << "No texture provided in updateVerticesBlt";
		return;
	}

	long x = m_x;
	long y = m_y;
	long xm = m_windowBuilder[0];
	long ym = m_windowBuilder[1];
	long wm = m_windowBuilder[2];
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
	RectSet(rect, 0, 0, w1, h1);
	calculateVertices(x, y, 0, 0, a, rect);
	// [ ..., ..., 3] / [ ..., ..., ...] / [ ..., ..., ...]
	a.x = wt - w3;
	RectSet(rect, ws - w3, 0, w3, h1);
	calculateVertices(x, y, 0, -1, a, rect);
	// [ ..., ..., ...] / [ ..., ..., ...] / [ ..., ..., 9]
	a.y = ht - h7;
	RectSet(rect, ws - w3, hs - h7, w3, h7);
	calculateVertices(x, y, m_vertices.size() - 1, -1, a, rect);
	// [ ..., ..., ...] / [ ..., ..., ...] / [ 7, ..., ...]
	a.x = 0;
	RectSet(rect, 0, hs - h7, w1, h7);
	calculateVertices(x, y, m_vertices.size() - 1, 0, a, rect);
	// Side vertice calculation
	int count;
	// [ ..., 2, ...] / [ ..., ..., ...] / [ ..., ..., ...]
	a.x = w1;
	a.y = 0;
	RectSet(rect, xm, 0, wm, h1);
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
	RectSet(rect, 0, ym, w1, hm);
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
	RectSet(rect, ws - w3, ym, w3, hm);
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
	RectSet(rect, xm, hs - h7, wm, h7);
	for (count = nb2; count > 0; count--)
	{
		calculateVertices(x, y, m_vertices.size() - 1, count, a, rect);
		a.x += wm;
	}
	if (delta_w > 0) {
		rect.width = delta_w;
		calculateVertices(x, y, m_vertices.size() - 1, -2, a, rect);
	}

	// Inside vertices calculation
	int count2;
	// [ ..., ..., ...] / [ ..., 5|m , ...] / [ ..., ..., ...]
	a.x = w1;
	a.y = h1;
	RectSet(rect, xm, ym, wm, hm);
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
		RectSet(rect, xm, ym, delta_w, delta_h);
		calculateVertices(x, y, m_vertices.size() - 2, -2, a, rect);
	}

	if (delta_h > 0) {
		a.x = w1;
		RectSet(rect, xm, ym, wm, delta_h);
		for (count = nb2; count > 0; count--) {
			calculateVertices(x, y, m_vertices.size() - 2, count, a, rect);
			a.x += wm;
		}
	}

	if (delta_w > 0) {
		a.y = h1;
		RectSet(rect, xm, ym, delta_w, hm);
		for (count = nb4; count > 0; count--)
		{
			calculateVertices(x, y, count, -2, a, rect);
			a.y += hm;
		}
	}
}

void cgss::FramedViewItem::setBuilder(std::array<long, 6> windowBuilder) {
	LOG_INFO << "Setting window builder 6";
	std::copy(std::begin(windowBuilder), std::begin(windowBuilder) + 6, std::begin(m_windowBuilder));
	m_windowBuilderExtended = false;
	updateVertices();
	updateView();
}

void cgss::FramedViewItem::setBuilder(std::array<long, 8> windowBuilder) {
	LOG_INFO << "Setting window builder 8";
	m_windowBuilder = std::move(windowBuilder);
	m_windowBuilderExtended = true;
	updateVertices();
	updateView();
}

long cgss::FramedViewItem::getPauseX() const {
	return m_pauseX.value_or(0);
}

long cgss::FramedViewItem::getPauseY() const {
	return m_pauseY.value_or(0);
}

void cgss::FramedViewItem::updateVerticesStretch(long wt, long ht) {
	sf::Vector2i a(0, 0);
	sf::Vector2i s(0, 0);
	sf::IntRect rect;

	if (m_texture == nullptr) {
		LOG_WARN << "No texture provided in updateVerticesStretch";
		return;
	}

	long x = m_x;
	long y = m_y;
	long xm = m_windowBuilder[0];
	long ym = m_windowBuilder[1];
	long wm = m_windowBuilder[2];
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
	RectSet(rect, 0, 0, w1, h1);
	calculateVertices(x, y, 0, 0, a, rect);
	// [ ..., ..., 3] / [ ..., ..., ...] / [ ..., ..., ...]
	a.x = wt - w3;
	RectSet(rect, ws - w3, 0, w3, h1);
	calculateVertices(x, y, 0, -1, a, rect);
	// [ ..., ..., ...] / [ ..., ..., ...] / [ ..., ..., 9]
	a.y = ht - h7;
	RectSet(rect, ws - w3, hs - h7, w3, h7);
	calculateVertices(x, y, m_vertices.size() - 1, -1, a, rect);
	// [ ..., ..., ...] / [ ..., ..., ...] / [ 7, ..., ...]
	a.x = 0;
	RectSet(rect, 0, hs - h7, w1, h7);
	calculateVertices(x, y, m_vertices.size() - 1, 0, a, rect);

	// 4 Side vertice calculation
	// [ ..., 2, ...] / [ ..., ..., ...] / [ ..., ..., ...]
	s.x = delta_w;
	s.y = h1;
	a.x = w1;
	a.y = 0;
	RectSet(rect, xm, 0, wm, h1);
	calculateVerticesStretch(x, y, 0, 1, s, a, rect);
	// [ ..., ..., ...] / [ ..., ..., ...] / [ ..., 8, ...]
	s.y = h7;
	a.x = w1;
	a.y = ht - h7;
	RectSet(rect, xm, hs - h7, wm, h7);
	calculateVerticesStretch(x, y, m_vertices.size() - 1, 1, s, a, rect);
	// [ ..., ..., ...] / [ 4, ..., ...] / [ ..., ..., ...]
	s.x = w1;
	s.y = delta_h;
	a.x = 0;
	a.y = h1;
	RectSet(rect, 0, ym, w1, hm);
	calculateVerticesStretch(x, y, 1, 0, s, a, rect);
	// [ ..., ..., ...] / [ ..., ..., 6] / [ ..., ..., ...]
	s.x = w3;
	a.x = wt - w3;
	a.y = h1;
	RectSet(rect, ws - w3, ym, w3, hm);
	calculateVerticesStretch(x, y, 1, 2, s, a, rect);

	// Middle vertice calculation
	s.x = delta_w;
	a.x = w1;
	a.y = h1;
	RectSet(rect, xm, ym, wm, hm);
	calculateVerticesStretch(x, y, 1, 1, s, a, rect);
}

void cgss::FramedViewItem::calculateVertices(long x, long y, long line, long cell, sf::Vector2i& a, sf::IntRect& rect) {
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

void cgss::FramedViewItem::calculateVerticesStretch(long x, long y, long line, long cell, sf::Vector2i& s, sf::Vector2i& a, sf::IntRect& rect) {
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

void cgss::FramedViewItem::updatePauseSprite() {
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

void cgss::FramedViewItem::updateCursorSprite() {
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

void cgss::FramedViewItem::allocateVerticesBlt(long delta_w, long nb2, long delta_h, long nb4) {
	// Ajustment of the real amount of cell in x
	if (delta_w > 0) {
		nb2 += 3;
	} else {
		nb2 += 2;
	}

	// Calculate the real amount of vertices
	nb2 *= 6;

	// Ajustment of the real amount of cell in y
	if (delta_h > 0) {
		nb4 += 3;
	} else {
		nb4 += 2;
	}
	// Delete the vertices if the number of line is different
	// Alloction of the vertex Arrays (if no vertices or different lines)
	if (m_vertices.empty() || nb4 != static_cast<long>(m_vertices.size())) {
		m_vertices = nb4 == 0 ? std::vector<sf::VertexArray>{} : std::vector<sf::VertexArray>(nb4);
	}

	for (auto& vertice : m_vertices) {
		vertice.setPrimitiveType(sf::Triangles);
		if (nb2 != static_cast<long>(vertice.getVertexCount())) {
			vertice.resize(nb2);
		}
	}
}

void cgss::FramedViewItem::allocateVerticesStretch() {
	allocateVerticesBlt(0, 1, 0, 1);
}

void cgss::FramedViewItem::updateContents() {
	setCursorRectangle(m_cursorRect);
}

void cgss::FramedViewItem::updateBackOpacity() {
	sf::Color col = sf::Color::White;
	col.a = m_opacity * m_backOpacity / 255;
	if (!m_vertices.empty()) {
		std::size_t num_vertices = m_vertices[0].getVertexCount();
		for (auto& vertArr : m_vertices) {
			for (std::size_t j = 0; j < num_vertices; j++) {
				vertArr[j].color = col;
			}
		}
	}
	updatePauseSprite();
	updateCursorSprite();
}

void cgss::FramedViewItem::setBackOpacity(uint8_t backOpacity) {
	m_backOpacity = backOpacity;
	updateBackOpacity();
}

std::uint8_t cgss::FramedViewItem::getBackOpacity() const {
	return m_backOpacity;
}

void cgss::FramedViewItem::updateView() {
	long offset_x = m_windowBuilder[4];
	long offset_y = m_windowBuilder[5];
	long offset_x2 = m_windowBuilderExtended ? m_windowBuilder[6] : offset_x;
	long offset_y2 = m_windowBuilderExtended ? m_windowBuilder[7] : offset_y;
	long x = static_cast<long>(m_x) + offset_x;
	long y = static_cast<long>(m_y) + offset_y;
	long width = static_cast<long>(m_wantedWidth) - offset_x - offset_x2;
	long height = static_cast<long>(m_wantedHeight) - offset_y - offset_y2;

	width = width < 0 ? 0 : width;
	height = height < 0 ? 0 : height;

	LOG_INFO << "View updated to x = " << x << " y = " << y << " width = " << width << " height = " << height;
	resize(width, height);
	moveViewport(x, y);
}

void cgss::FramedViewItem::lock() {
	m_locked = true;
	LOG_INFO << "Locked";
}

void cgss::FramedViewItem::unlock() {
	m_locked = false;
	LOG_INFO << "Unlocked, updating vertices";
	updateVertices();
}

bool cgss::FramedViewItem::isLocked() const {
	return m_locked;
}

const sf::IntRect& cgss::FramedViewItem::getCursorRectangle() const {
	return m_cursorRect;
}

void cgss::FramedViewItem::resetPausePosition() {
	auto size = m_pause.getTexture() == nullptr ? sf::Vector2u{} : m_pause.getTexture()->getSize();
	float pause_x = m_pauseX.has_value() ? static_cast<float>(m_pauseX.value()) : (m_wantedWidth - size.x) / 2.F;
	float pause_y = m_pauseY.has_value() ? static_cast<float>(m_pauseY.value()) : (m_wantedHeight - size.y) / 2.F;
	pause_x += m_x;
	pause_y += m_y;
	m_pause.setPosition(sf::Vector2f(pause_x, pause_y));
}

void cgss::FramedViewItem::setCursorRectangle(sf::IntRect rect) {
	setValue(rect);
	m_cursorRect = std::move(rect);

	if (m_cursor.getTexture() == nullptr) {
		return;
	}
	sf::Vector2f size = static_cast<sf::Vector2f>(m_cursor.getTexture()->getSize());
	m_cursor.setScale(sf::Vector2f(m_cursorRect.width / size.x, m_cursorRect.height / size.y));
	m_cursor.setPosition(sf::Vector2f(static_cast<float>(m_cursorRect.left), static_cast<float>(m_cursorRect.top)));
}

void cgss::FramedViewItem::updateContentsOpacity() {
	const auto opacity = static_cast<uint8_t>((m_opacity * m_contentOpacity) / 255.F);

	drawables().updateContentsOpacity(opacity);
}

void cgss::FramedViewItem::setPauseSkin(sf::Texture& texture) {
	m_pause.setTexture(texture);
	resetPausePosition();
	updatePauseSprite();
}

void cgss::FramedViewItem::pause(bool pause) {
	m_paused = pause;
	updatePauseSprite();
}

void cgss::FramedViewItem::setPausePosition(float x, float y) {
	m_pauseX = static_cast<long>(x);
	m_pauseY = static_cast<long>(y);
	resetPausePosition();
}

bool cgss::FramedViewItem::isPaused() const {
	return m_paused;
}

void cgss::FramedViewItem::setCursorSkin(sf::Texture& texture) {
	m_cursor.setTexture(texture);
	setCursorRectangle(m_cursorRect);
	updateCursorSprite();
}

void cgss::FramedViewItem::active(bool active) {
	m_active = active;
	updateCursorSprite();
}

bool cgss::FramedViewItem::isActive() const {
	return m_active;
}

void cgss::FramedViewItem::onMove() {
	updateVertices();
	updateView();
}

void cgss::FramedViewItem::stretch(bool stretch) {
	m_stretch = stretch;
	updateVertices();
}

bool cgss::FramedViewItem::isStretched() const {
	return m_stretch;
}

void cgss::FramedViewItem::onResize() {
	updateVertices();
	updateView();
}

void cgss::FramedViewItem::setOpacity(uint8_t opacity) {
	m_opacity = opacity;
	updateBackOpacity();
	updateContentsOpacity();
}

uint8_t cgss::FramedViewItem::getOpacity() const {
	return m_opacity;
}

void cgss::FramedViewItem::setContentsOpacity(uint8_t opacity) {
	m_contentOpacity = opacity;
	updateContentsOpacity();
}

uint8_t cgss::FramedViewItem::getContentsOpacity() const {
	return m_contentOpacity;
}

void cgss::FramedViewItem::setSkin(sf::Texture* skin) {
	m_texture = skin;
	LOG_INFO << "Skin set";
	updateVertices();
}

sf::Texture* cgss::FramedViewItem::getSkin() const {
	return m_texture;
}

void cgss::FramedViewItem::move(float x, float y) {
	m_x = x;
	m_y = y;
	updateView();
}

void cgss::FramedView::move(float x, float y) {
	m_proxy->move(x, y);
}

void cgss::FramedView::lock() {
	m_proxy->lock();
}

void cgss::FramedView::unlock() {
	m_proxy->unlock();
}

bool cgss::FramedView::isLocked() const {
	return m_proxy->isLocked();
}

void cgss::FramedView::setOpacity(uint8_t opacity) {
	m_proxy->setOpacity(opacity);
}

uint8_t cgss::FramedView::getOpacity() const {
	return m_proxy->getOpacity();
}

void cgss::FramedView::setContentsOpacity(uint8_t opacity) {
	m_proxy->setContentsOpacity(opacity);
}

uint8_t cgss::FramedView::getContentsOpacity() const {
	return m_proxy->getContentsOpacity();
}

void cgss::FramedView::pause(bool pause) {
	m_proxy->pause(pause);
}

bool cgss::FramedView::isPaused() const {
	return m_proxy->isPaused();
}

void cgss::FramedView::active(bool active) {
	m_proxy->active(active);
}

bool cgss::FramedView::isActive() const {
	return m_proxy->isActive();
}

void cgss::FramedView::setSkin(sf::Texture* skin) {
	return m_proxy->setSkin(skin);
}

sf::Texture* cgss::FramedView::getSkin() const {
	return m_proxy->getSkin();
}

void cgss::FramedView::setPauseSkin(sf::Texture& texture) {
	m_proxy->setPauseSkin(texture);
}

void cgss::FramedView::setPausePosition(float x, float y) {
	m_proxy->setPausePosition(x, y);
}

void cgss::FramedView::setCursorSkin(sf::Texture& texture) {
	m_proxy->setCursorSkin(texture);
}

void cgss::FramedView::setCursorRectangle(sf::IntRect position) {
	m_proxy->setCursorRectangle(std::move(position));
}

void cgss::FramedView::stretch(bool stretch) {
	m_proxy->stretch(stretch);
}

bool cgss::FramedView::isStretched() const {
	return m_proxy->isStretched();
}

void cgss::FramedView::draw(sf::RenderTarget& target) const {
	m_proxy->drawFast(target);
}

void cgss::FramedView::drawFast(sf::RenderTarget& target) const {
	m_proxy->draw(target);
}

void cgss::FramedView::sortZ() {
	m_proxy->sortZ();
}

void cgss::FramedView::bindRectangleViewport(Rectangle* output) {
	m_proxy->bindRectangleViewport(output);
}

void cgss::FramedView::moveOrigin(long ox, long oy) {
	m_proxy->m_wantedOx = ox;
	m_proxy->m_wantedOy = oy;
	m_proxy->moveOrigin(ox, oy);
}

void cgss::FramedView::setAngle(float angle) {
	m_proxy->setAngle(angle);
}

void cgss::FramedView::resize(long width, long height) {
	m_proxy->m_wantedWidth = width;
	m_proxy->m_wantedHeight = height;
	m_proxy->resize(width, height);
}

void cgss::FramedView::setBuilder(std::array<long, 6> windowBuilder) {
	m_proxy->setBuilder(windowBuilder);
}

void cgss::FramedView::setBuilder(std::array<long, 8> windowBuilder) {
	m_proxy->setBuilder(windowBuilder);
}

void cgss::FramedViewItem::updateFromValue(const sf::IntRect* rectangle) {
	setCursorRectangle(rectangle == nullptr ? sf::IntRect{} : *rectangle);
}

long cgss::FramedView::getPauseX() const {
	return m_proxy->getPauseX();
}

long cgss::FramedView::getPauseY() const {
	return m_proxy->getPauseY();
}

const sf::IntRect& cgss::FramedView::getCursorRectangle() const {
	return m_proxy->getCursorRectangle();
}

sf::IntRect cgss::FramedView::getViewportBox() const {
	return m_proxy->getRectangleBox();
}

float cgss::FramedView::getX() const {
	return m_proxy->m_x;
}

float cgss::FramedView::getY() const {
	return m_proxy->m_y;
}

float cgss::FramedView::getWidth() const {
	return m_proxy->m_wantedWidth;
}

float cgss::FramedView::getHeight() const {
	return m_proxy->m_wantedHeight;
}

long cgss::FramedView::getOx() const {
	return m_proxy->m_wantedOx;
}

long cgss::FramedView::getOy() const {
	return m_proxy->m_wantedOy;
}

void cgss::FramedView::update() {
	m_proxy->update();
}

void cgss::FramedView::setBackOpacity(uint8_t backOpacity) {
	m_proxy->setBackOpacity(backOpacity);
}

std::uint8_t cgss::FramedView::getBackOpacity() const {
	return m_proxy->getBackOpacity();
}
