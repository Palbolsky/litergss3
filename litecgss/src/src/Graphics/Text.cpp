#include "Text.h"

void cgss::Text::draw(sf::RenderTarget& target) const {
	drawFast(target);
}

void cgss::Text::drawFast(sf::RenderTarget& target) const {
	target.draw(m_text);
}
