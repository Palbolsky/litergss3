#include "Sprite.h"

void cgss::Sprite::draw(sf::RenderTarget& target) const {
	Sprite::drawFast(target);
}

void cgss::Sprite::drawFast(sf::RenderTarget& target) const {
	if(m_drawable && m_visible) {
		target.draw(m_sprite);
	}
}
