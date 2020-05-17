#include "Text.h"

void cgss::TextItem::draw(sf::RenderTarget& target) const {
	drawFast(target);
}

void cgss::TextItem::drawFast(sf::RenderTarget& target) const {
	target.draw(m_text);
}
