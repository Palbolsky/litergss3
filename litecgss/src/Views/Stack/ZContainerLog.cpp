#include "LiteCGSS/Views/Stack/ZBaseContainer.h"
#include "Common/Log.h"

CGSS_LOG_COMPONENT("ZContainer", err)

void cgss::detail::LogZRemoveMiss(const void* container, const ZIndex& itemZ, std::size_t position) {
	CGSS_LOG_ERROR("{} Trying to remove an element from stack but unable to find it for z-index = {}, {} only found position = {} but no item equality",
		container, itemZ.z, itemZ.index, position);
}
