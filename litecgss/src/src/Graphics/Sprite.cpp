#include "Sprite.h"

cgss::Sprite::Sprite() : 
	Transformable(m_sprite) {
}

void cgss::Sprite::draw(sf::RenderTarget& target) const {
	Sprite::drawFast(target);
}

void cgss::Sprite::drawFast(sf::RenderTarget& target) const {
	if(m_visible) {
		target.draw(m_sprite, m_renderStates);
	}
}

void cgss::Sprite::setRenderStates(sf::RenderStates renderStates) {
	m_renderStates = std::move(renderStates);
}

void cgss::Sprite::setMirror(bool mirror) {
	if (m_mirror ^ mirror) {
		horizontalFlip();
		m_mirror = mirror;
	}
}

void cgss::Sprite::horizontalFlip() {
	auto rect = m_sprite.getTextureRect();
	rect.left += rect.width;
	rect.width = -rect.width;
	m_sprite.setTextureRect(rect);
}

sf::Color cgss::Sprite::getColor() const {
	return m_sprite.getColor();
}

void cgss::Sprite::setColor(sf::Color color) {
	m_sprite.setColor(color);
}
