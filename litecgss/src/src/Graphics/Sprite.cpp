#include <cassert>
#include "Sprite.h"
#include "Texture.h"
#include "Common/Rectangle.h"

cgss::SpriteItem::SpriteItem(Sprite* owner) {
	owner->setInstance(m_sprite);
}

void cgss::SpriteItem::draw(sf::RenderTarget& target) const {
	SpriteItem::drawFast(target);
}

void cgss::SpriteItem::drawFast(sf::RenderTarget& target) const {
	target.draw(m_sprite, m_renderStates);
}

void cgss::Sprite::setRenderStates(sf::RenderStates renderStates) {
	m_proxy->m_renderStates = std::move(renderStates);
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
	m_proxy->m_sprite.setTextureRect(rect);
}

sf::Color cgss::Sprite::getColor() const {
	return m_proxy->m_sprite.getColor();
}

void cgss::Sprite::setColor(sf::Color color) {
	m_proxy->m_sprite.setColor(color);
}

void cgss::Sprite::setTexture(Texture& texture, bool resetRect) {
	m_proxy->m_sprite.setTexture(texture.getTexture());
	if (resetRect) {
		setTextureRect(sf::IntRect{});
	}
}

void cgss::SpriteItem::updateFromRectangle(const sf::IntRect& rectangle) {
	auto tmpRectangle = rectangle;
	if (m_mirror) {
		tmpRectangle.left += tmpRectangle.width;
		tmpRectangle.width = -tmpRectangle.width;
	}
	m_sprite.setTextureRect(std::move(tmpRectangle));
}

void cgss::Sprite::setTextureRect(sf::IntRect rectangle) {
	auto* boundRectangle = m_proxy->getBoundRectangle();
	if (boundRectangle != nullptr) {
		boundRectangle->setRect(rectangle);
	} else {
		m_proxy->updateFromRectangle(rectangle);
	}
}

const sf::IntRect& cgss::Sprite::getTextureRect() const {
	return m_proxy->m_sprite.getTextureRect();
}
