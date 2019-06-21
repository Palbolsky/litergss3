#include "GraphicsStack.h"
#include "GraphicsStackItem.h"

lr::GraphicsStack::~GraphicsStack() {
	clear();
}

void lr::GraphicsStack::add(GraphicsStackItem& el) {
	el.setOriginStack(&m_stack);
}

void lr::GraphicsStack::remove(GraphicsStackItem& el) {
	el.setOriginStack(nullptr);
}

void lr::GraphicsStack::clear() {
	syncFromRawData({});
}
/*
void lr::GraphicsStack::syncStackCppFromRuby() {
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

void lr::GraphicsStack::syncFromRawData(std::vector<GraphicsStackItem*> data, bool overrideOriginStack) {
	if(overrideOriginStack) {
		for(auto& it : m_stack) {
			it->overrideOriginStack();
		}
	}
	m_stack = std::move(data);
}
