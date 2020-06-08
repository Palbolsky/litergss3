#include "DrawableStack.h"
#include "Graphics/Text.h"
#include "Graphics/Sprite.h"
#include "LoggerStack.h"

SKA_LOGC_CONFIG(ska::LogLevel::Debug, cgss::DrawableStack);

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerStack(), ska::LogLevel::Debug, cgss::DrawableStack)
#define LOG_INFO SKA_LOGC_STATIC(GetLoggerStack(), ska::LogLevel::Info, cgss::DrawableStack)
#define LOG_WARN SKA_LOGC_STATIC(GetLoggerStack(), ska::LogLevel::Warn, cgss::DrawableStack)

static std::string PrintStack(const cgss::DrawableStack& drawables) {
	auto ss = std::stringstream {};
	for (const auto& element : drawables) {
		ss << "[" << element->getZ().z  << ", " << element->getZ().index << "]";
	}
	return ss.str();
}

void cgss::DrawableStack::draw(sf::View& defview, sf::RenderTarget& target) const  {
	static int counter = 60;
	if (++counter % 120 == 0) { 
		LOG_DEBUG << PrintStack(*this);
		counter = 0;
	}

	for(auto& element: *this) {		
		target.setView(defview);
		element->tryDraw(target);
	}
}

void cgss::DrawableStack::drawFast(sf::RenderTarget& target) const {
	static int counter = 0;
	if (++counter % 120 == 0) { 
		LOG_DEBUG << PrintStack(*this);
		counter = 0;
	}

	for(auto& element: *this) {
		element->tryDrawFast(target);
	}
}

void cgss::DrawableStack::updateContentsOpacity(std::uint8_t opacity) {
	for (auto& sp : *this) {
		//TODO polymorphism

		Text* text = dynamic_cast<Text*>(sp.get());
		if (text != nullptr) {
			text->setOpacity(opacity);
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
