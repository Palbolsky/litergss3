#include "DrawableStack.h"
#include "Graphics/Text.h"
#include "Graphics/Sprite.h"

void lr::DrawableStack::draw(sf::View& defview, sf::RenderTarget& target) const  {
	//bool was_viewport = false;
	for(auto& element: *this) {
		//if(was_viewport && !element->isViewport()) {
			target.setView(defview);
		//}
		//was_viewport = element->isViewport();
		element->draw(target);
	}
	freeDetached();
}

void lr::DrawableStack::drawFast(sf::RenderTarget& target) const {
	for(auto& element: *this) {
		element->drawFast(target);
	}
	freeDetached();
}

void lr::DrawableStack::updateContentsOpacity(std::uint8_t opacity) {
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
				sf::Color col = sf::Color(sprite->getSprite().getColor());
				col.a = opacity;
				sprite->getSprite().setColor(col);
			}
		}
	}
}
