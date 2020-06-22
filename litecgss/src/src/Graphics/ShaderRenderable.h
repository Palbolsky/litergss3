#ifndef CGSS_SHADER_RENDERABLE_H
#define CGSS_SHADER_RENDERABLE_H

namespace cgss {
    class RenderStates;

    class ShaderRenderable {
    private:
        RenderStates* m_target = nullptr;
    public:
        virtual ~ShaderRenderable();
        void bindRenderStates(RenderStates* target);
        const RenderStates* getBoundRenderStates() const { return m_target; }
    protected:
        RenderStates* getBoundRenderStates() { return m_target; }
    };
}

#endif
