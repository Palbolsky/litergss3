#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/View.hpp>
#include "DrawableStack.h"
#include "LiteCGSS/Graphics/Text.h"
#include "LiteCGSS/Graphics/Sprite.h"

#include <Logging/PeriodicTracer.h>
#include "LoggerStack.h"

SKA_LOGC_CONFIG(ska::LogLevel::Warn, cgss::DrawableStack)
#define LOG_FREQUENCY 300

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerStack(), ska::LogLevel::Debug, cgss::DrawableStack)
#define LOG_INFO SKA_LOGC_STATIC(GetLoggerStack(), ska::LogLevel::Info, cgss::DrawableStack)
#define LOG_WARN SKA_LOGC_STATIC(GetLoggerStack(), ska::LogLevel::Warn, cgss::DrawableStack)

#define LOG_FREQUENCY 300
static ska::PeriodicTracer<cgss::LoggerStack, ska::LogLevel::Debug> DrawableStackTracer = { cgss::GetLoggerStack() };

void cgss::DrawableStack::draw(cgss::Camera& defview, cgss::RenderTarget& target) const  {
	DrawableStackTracer.SKA_LOG_PERIODIC_TRACE(LOG_FREQUENCY);
	for (auto& element: *this) {
		target.raw().setView(defview.raw());
		element->tryDraw(target);
	}
}

void cgss::DrawableStack::drawFast(cgss::RenderTarget& target) const {
	DrawableStackTracer.SKA_LOG_PERIODIC_TRACE(LOG_FREQUENCY);
	for (auto& element: *this) {
		element->tryDrawFast(target);
	}
}

void cgss::DrawableStack::updateContentsOpacity(std::uint8_t opacity) {
	for (auto& sp : *this) {
		sp->setOpacity(opacity);
	}
}
