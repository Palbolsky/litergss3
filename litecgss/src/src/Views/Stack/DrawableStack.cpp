#include "DrawableStack.h"
#include "Graphics/Text.h"
#include "Graphics/Sprite.h"

void cgss::DrawableStack::draw(sf::View& defview, sf::RenderTarget& target) const  {
	for(auto& element: *this) {		
		target.setView(defview);
		element->tryDraw(target);
	}
}

void cgss::DrawableStack::drawFast(sf::RenderTarget& target) const {
	for(auto& element: *this) {
		element->tryDrawFast(target);
	}
}

void cgss::DrawableStack::updateContentsOpacity(std::uint8_t opacity) {
	for (auto& sp : *this) {
		//TODO polymorphism

		Text* text = dynamic_cast<Text*>(sp.get());
		if (text != nullptr) {
			auto& text2 = text->getText();
			sf::Color col = sf::Color(text2.getFillColor());
			col.a = opacity;
			text2.setFillColor(col);
			col = sf::Color(text2.getOutlineColor());
			col.a = opacity;
			text2.setOutlineColor(col);
		} else {
			Sprite* sprite = dynamic_cast<Sprite*>(sp.get());
			if (sprite != nullptr) {
				sf::Color col = sprite->getColor();
				col.a = opacity;
				sprite->setColor(col);
			}
		}
	}
}
