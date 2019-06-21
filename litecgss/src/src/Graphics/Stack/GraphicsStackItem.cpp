#include "GraphicsStackItem.h"
#include "GraphicsStack.h"

void lr::GraphicsStackItem::setOriginStack(GraphicsContainer *o) {
	/* if the stack is already setted, nothing to be done */
	if(m_originStack == o) {
		return;
	}

	//Detach & Attach C++
	if(m_originStack != nullptr) {
		m_originStack->remove(this);
	}
	m_originStack = o;
	if(m_originStack != nullptr) {
		m_originStack->push_back(this);
		if(m_drawPriority == 0) {
			m_drawPriority = m_originStack->size();
		}
	}
}
