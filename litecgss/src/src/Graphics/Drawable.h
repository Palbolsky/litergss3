#ifndef CGSS_DRAWABLE_H
#define CGSS_DRAWABLE_H
#include <memory>
#include "GraphicsStackItem.h"
#include "Views/Stack/DrawableStack.h"

namespace cgss {
	class Rectangle;
	class Tone;

	template <class DrawableOwner, class StackItem>
	class Drawable {
		
	public:
		virtual ~Drawable() = default;
	
		void dispose() {
			if (m_instanceOwner != nullptr && !m_disposed) {
				m_instanceOwner->detach();
				m_disposed = true;
				onDispose();				
			}
		}

		const ZIndex& getZ() const {
			return m_instanceOwner->getZ();
		}
		
		void setZ(ZIndex z) {
			m_instanceOwner->setZ(std::move(z));
		}

		void setZ(long z) {
			m_instanceOwner->setZ(z);
		}

		void setVisible(bool visible) {
			m_instanceOwner->setVisible(visible);
		}

		bool isVisible() const {
			return m_instanceOwner->isVisible();
		}

		void bindRectangle(Rectangle* rectangle) {
			m_instanceOwner->bindRectangle(rectangle);
		}

		template <class View, class ... Args>
		static DrawableOwner create(View& view, Args&& ... args) {
			auto result = buildOwner(std::forward<Args>(args)...);
			view.template add<View, StackItem, DrawableOwner>(result.m_instanceOwner);
			return std::move(result);
		}

		template <class ... Args>
		static DrawableOwner create(DrawableStack& stack, Args&& ... args) {
			auto result = buildOwner(std::forward<Args>(args)...);
			stack.add(result.m_instanceOwner);
			return std::move(result);
		}

	protected:
		Drawable() {
			static_assert(std::is_base_of<GraphicsStackItem, StackItem>::value, "StackItem must inherit GraphicStackItem");
		}

		Drawable(Drawable&& drawable) noexcept {
			*this = std::move(drawable);
		}

		Drawable& operator=(Drawable&& drawable) noexcept {
			m_instanceOwner = std::move(drawable.m_instanceOwner);
			m_proxy = m_instanceOwner.get();
			m_disposed = drawable.m_disposed;
			drawable.m_proxy = nullptr;
			drawable.m_instanceOwner = nullptr;
			return *this;
		}

		Drawable(const Drawable&) = delete;
		Drawable& operator=(const Drawable&) = delete;

		StackItem* m_proxy = nullptr;
	
		bool hasProxy() const { return m_proxy != nullptr && m_instanceOwner != nullptr; }
		
		virtual void onDispose() {}

	private:
		template <class ... Args>
		static DrawableOwner buildOwner(Args&& ... args) {
			auto result = DrawableOwner{ };
			result.m_instanceOwner = std::shared_ptr<StackItem>{ new StackItem(&result, std::forward<Args>(args)...) };
			result.m_proxy = result.m_instanceOwner.get();
			return result;
		}

		std::shared_ptr<StackItem> m_instanceOwner = nullptr;
		bool m_disposed = false;
	};
	
}
#endif