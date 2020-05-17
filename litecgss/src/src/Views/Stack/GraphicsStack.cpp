#include "GraphicsStack.h"
#include "Graphics/GraphicsStackItem.h"

cgss::GraphicsStack::~GraphicsStack() {
	clear();
}

void cgss::GraphicsStack::clear() {
	syncFromRawData({});
}

/*
void cgss::GraphicsStack::syncStackCppFromRuby() {
	//MUST BE MOVED TO BINDING PART

	const long sz = RARRAY_LEN(rubyStack.self());
	VALUE* ori = RARRAY_PTR(rubyStack.self());
	auto data = std::vector<CGraphicsStackItem*>{};
	if(sz > 0) {
		data.reserve(sz);
	}
	for(long  i = 0; i < sz; i++) {
		const auto it = ori[i];
		if(rb_obj_is_kind_of(it, rb_cDrawable) == Qtrue && RDATA(it)->data != nullptr) {
			auto& element = *reinterpret_cast<CGraphicsStackItem*>(RDATA(it)->data);
			element.overrideOriginCppStack(&cppStack);
			data.push_back(&element);
		}
	}
	//END MOVE TO BINDING PART

	syncFromRawData(std::move(data), false);

}
*/

void cgss::GraphicsStack::syncFromRawData(GraphicsContainer data, bool forceDetachAll) {
	if(forceDetachAll) {
		for(auto& it : m_stack) {
			it->forceDetach();
		}
	}
	m_stack = std::move(data);
	freeDetached();
}

bool cgss::GraphicsStack::transferToDetached(GraphicsStackItem* item) {
	auto removed = m_stack.remove_if([&](const auto& drawablePtr) {
		return drawablePtr.get() == item;
	});

	if (removed.has_value()) {
		auto lock = std::scoped_lock{ m_mutexDetached };
		m_detached.push_back(removed.value());
		return true;
	}
	return false;
}

void cgss::GraphicsStack::freeDetached() const {
	auto lock = std::scoped_lock{ m_mutexDetached };
	m_detached = {};
}

void cgss::GraphicsStack::add(GraphicsStackItemPtr item) {
	item->forceAttach(*this);
	m_stack.push_back(std::move(item));
}
