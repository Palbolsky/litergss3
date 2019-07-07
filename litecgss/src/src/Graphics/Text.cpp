#include "Text.h"

void lr::Text::draw(sf::RenderTarget& target) const {
	drawFast(target);
}

void lr::Text::drawFast(sf::RenderTarget& target) const {
	if(m_visible) {
		target.draw(m_text);
	}
}
