#include "DrawableStack.h"
#include "LiteCGSS/Graphics/Text.h"
#include "LiteCGSS/Graphics/Sprite.h"

#include <Logging/PeriodicTracer.h>
#include "LoggerStack.h"

SKA_LOGC_CONFIG(ska::LogLevel::Warn, cgss::DrawableStack);

#define LOG_FREQUENCY 300
static ska::PeriodicTracer<cgss::LoggerStack, ska::LogLevel::Debug> DrawableStackTracer = { cgss::GetLoggerStack() };

void cgss::DrawableStack::draw(sf::View& defview, sf::RenderTarget& target) const  {
	DrawableStackTracer.SKA_LOG_PERIODIC_TRACE(LOG_FREQUENCY);
	for(auto& element: *this) {		
		target.setView(defview);
		element->tryDraw(target);
	}
}

void cgss::DrawableStack::drawFast(sf::RenderTarget& target) const {
	DrawableStackTracer.SKA_LOG_PERIODIC_TRACE(LOG_FREQUENCY);
	for(auto& element: *this) {
		element->tryDrawFast(target);
	}
}

void cgss::DrawableStack::updateContentsOpacity(std::uint8_t opacity) {
	for (auto& sp : *this) {
		sp->setOpacity(opacity);
	}
}
