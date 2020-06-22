#include "DrawableStack.h"
#include "Graphics/Text.h"
#include "Graphics/Sprite.h"
#include "LoggerStack.h"

SKA_LOGC_CONFIG(ska::LogLevel::Warn, cgss::DrawableStack);
#define LOG_FREQUENCY 300

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerStack(), ska::LogLevel::Debug, cgss::DrawableStack)
#define LOG_INFO SKA_LOGC_STATIC(GetLoggerStack(), ska::LogLevel::Info, cgss::DrawableStack)
#define LOG_WARN SKA_LOGC_STATIC(GetLoggerStack(), ska::LogLevel::Warn, cgss::DrawableStack)

static std::string PrintStack(const cgss::DrawableStack& drawables) {
	auto ss = std::stringstream {};
	ss << static_cast<const void*>(&drawables) << "\t";
	for (const auto& element : drawables) {
		ss << "[" << element->getZ().z  << ", " << element->getZ().index << "]";
	}
	return ss.str();
}

void cgss::DrawableStack::draw(sf::View& defview, sf::RenderTarget& target) const  {
	static int counter = LOG_FREQUENCY / 2;
	if (++counter % LOG_FREQUENCY == 0) { 
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
	if (++counter % LOG_FREQUENCY == 0) { 
		LOG_DEBUG << PrintStack(*this);
		counter = 0;
	}

	for(auto& element: *this) {
		element->tryDrawFast(target);
	}
}

void cgss::DrawableStack::updateContentsOpacity(std::uint8_t opacity) {
	for (auto& sp : *this) {
		sp->setOpacity(opacity);
	}
}
