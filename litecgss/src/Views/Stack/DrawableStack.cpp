#include "LiteCGSS/Views/Stack/DrawableStack.h"
#include "Common/Profiling.h"
#include "Backend/ActiveBackend.h"
#include "LiteCGSS/Graphics/Text.h"
#include "LiteCGSS/Graphics/Sprite.h"

#include <Logging/PeriodicTracer.h>
#include "LiteCGSS/Views/Stack/LoggerStack.h"

SKA_LOGC_CONFIG(ska::LogLevel::Warn, cgss::DrawableStack)
#define LOG_FREQUENCY 300

#define LOG_DEBUG SKA_LOGC_STATIC(GetLoggerStack(), ska::LogLevel::Debug, cgss::DrawableStack)
#define LOG_INFO SKA_LOGC_STATIC(GetLoggerStack(), ska::LogLevel::Info, cgss::DrawableStack)
#define LOG_WARN SKA_LOGC_STATIC(GetLoggerStack(), ska::LogLevel::Warn, cgss::DrawableStack)

#define LOG_FREQUENCY 300
static ska::PeriodicTracer<cgss::LoggerStack, ska::LogLevel::Debug> DrawableStackTracer = { cgss::GetLoggerStack() };

void cgss::DrawableStack::draw(cgss::Camera& defview, cgss::RenderTarget& target) const  {
	CGSS_PROFILE_ZONE("cgss::DrawableStack::draw");
	using Ops = cgss::backend::ActiveBackend::Ops;
	DrawableStackTracer.SKA_LOG_PERIODIC_TRACE(LOG_FREQUENCY);
	// Apply the default view once. View-mutating items (ViewportItem,
	// FramedViewItem) save+restore the incoming view themselves, so it is
	// invariant across iterations — no per-element re-set needed.
	Ops::target_set_view(cgss::backend::native_of(target), cgss::backend::native_of(defview));
	for (auto& element: *this) {
		element->tryDraw(target);
	}
}

void cgss::DrawableStack::drawFast(cgss::RenderTarget& target) const {
	CGSS_PROFILE_ZONE("cgss::DrawableStack::drawFast");
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
