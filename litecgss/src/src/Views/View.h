#ifndef CGSS_VIEW_H
#define CGSS_VIEW_H
#include <cassert>
#include <memory>
#include <vector>

#include "Graphics/Drawable.h"
#include "Views/Stack/DrawableStack.h"
#include "ViewAuthorizations.h"

namespace cgss {

	//We only expose the "add" method to the internal of Drawable class
	class View;

	class ViewAdder {
		template <class T>
		friend class Drawable;
	private:
		template <class ViewImpl, class DrawableChild>
		DrawableChild& add(std::unique_ptr<DrawableChild> d) {
			static_assert(std::is_base_of<GraphicsStackItem, DrawableChild>::value, "Cannot add this class to a view : it's not a child class of GraphicsStackItem");
			static_assert(DrawableIsAuthorizedOnView<DrawableChild, ViewImpl>::value, "Unable to add this drawable : his type is not authorized to be attached on this kind of view.");
			assert(d != nullptr);
			auto& ref = *d.get();
			static_cast<View&>(*this).m_drawables.add(std::move(d));
			return ref;
		}
	};

	class View : 
		public ViewAdder {
		friend class ViewAdder;
	public:
		View() = default;
		virtual ~View() = default;

		auto getSize() { return m_drawables.getSize(); }
	protected:
		void draw(sf::View& view, sf::RenderTarget& target) const { m_drawables.draw(view, target); }
		void draw(sf::RenderTarget& target) const { m_drawables.drawFast(target); }

		void updateContentsOpacity(std::uint8_t opacity) { m_drawables.updateContentsOpacity(opacity); }
		void syncFromRawData(GraphicsContainer data, bool forceDetachAll = true) { m_drawables.syncFromRawData(std::move(data), forceDetachAll); }
	private:
		DrawableStack m_drawables;
	};
}

#endif