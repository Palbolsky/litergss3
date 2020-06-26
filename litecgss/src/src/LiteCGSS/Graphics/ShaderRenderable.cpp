#include "RenderStates.h"
#include "ShaderRenderable.h"

void cgss::ShaderRenderable::bindRenderStates(RenderStates* target) {
    if (target != m_target) {
        auto lastLinkedElement = m_target;
        m_target = nullptr;
        if (lastLinkedElement != nullptr) {
            lastLinkedElement->bindElement(nullptr);
        }
        m_target = target;
        if (m_target != nullptr) {
            m_target->bindElement(this);
        }
    }
}

cgss::ShaderRenderable::~ShaderRenderable() {
    bindRenderStates(nullptr);
}
