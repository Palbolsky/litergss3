#include "Sprite.h"

void lr::Sprite::draw(sf::RenderTarget& target) const {
	Sprite::drawFast(target);
}

void lr::Sprite::drawFast(sf::RenderTarget& target) const {
	if(m_drawable && m_visible) {
		target.draw(m_sprite);
	}
}
