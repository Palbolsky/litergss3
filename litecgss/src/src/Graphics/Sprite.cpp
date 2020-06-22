#include <cassert>
#include "Sprite.h"
#include "RenderStates.h"
#include "Texture.h"
#include "Common/Rectangle.h"
#include "Common/Meta/metadata.h"

cgss::SpriteItem::SpriteItem(Sprite* owner) {
	owner->setInstance(m_sprite);
}

void cgss::SpriteItem::draw(sf::RenderTarget& target) const {
	SpriteItem::drawFast(target);
}

void cgss::SpriteItem::drawFast(sf::RenderTarget& target) const {
	target.draw(m_sprite, getBoundRenderStates() == nullptr ? sf::RenderStates::Default : getBoundRenderStates()->getRenderStates());	
}

void cgss::Sprite::bindRenderStates(RenderStates* renderStates) {
	m_proxy->bindRenderStates(renderStates);
}

void cgss::Sprite::setMirror(bool mirror) {
	if (m_proxy->m_mirror ^ mirror) {
		horizontalFlip();
		m_proxy->m_mirror = mirror;
	}
}

void cgss::Sprite::horizontalFlip() {
	auto rect = m_proxy->m_sprite.getTextureRect();
	rect.left += rect.width;
	rect.width = -rect.width;
	setTextureRect(rect);
}

sf::Color cgss::Sprite::getColor() const {
	return m_proxy->m_sprite.getColor();
}

float cgss::Sprite::getX() const {
	return m_proxy->m_wantedX;
}

float cgss::Sprite::getY() const {
	return m_proxy->m_wantedY;
}

void cgss::Sprite::move(float x, float y) {
	m_proxy->m_wantedX = x;
	m_proxy->m_wantedY = y;
	Transformable::move(x, y);
}

void cgss::Sprite::setColor(sf::Color color) {
	m_proxy->m_sprite.setColor(std::move(color));
}

void cgss::SpriteItem::setOpacity(std::uint8_t opacity) {
	sf::Color col = m_sprite.getColor();
	col.a = opacity;
	m_sprite.setColor(col);
}

void cgss::Sprite::setTexture(Texture& texture, bool resetRect) {
	m_proxy->m_sprite.setTexture(texture.getTexture(), resetRect);
	if (resetRect) {
		m_proxy->setValue(m_proxy->m_sprite.getTextureRect());
	}
}

void cgss::SpriteItem::updateFromValue(const sf::IntRect* rectangle) {
	auto tmpRectangle = rectangle == nullptr ? sf::IntRect{} : *rectangle;
	if (m_mirror) {
		tmpRectangle.left += tmpRectangle.width;
		tmpRectangle.width = -tmpRectangle.width;
	}
	m_sprite.setTextureRect(std::move(tmpRectangle));
}

void cgss::Sprite::setTextureRect(sf::IntRect rectangle) {
	m_proxy->setValue(std::move(rectangle));
}

const sf::IntRect& cgss::Sprite::getTextureRect() const {
	return m_proxy->m_sprite.getTextureRect();
}
