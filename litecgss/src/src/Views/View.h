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
	template <class View>
	class ViewAdder {
		template <class Owner, class Item>
		friend class Drawable;
	private:
		template <class ViewImpl, class StackItem, class Drawable>
		StackItem& add(std::shared_ptr<StackItem> d) {
			static_assert(std::is_base_of<GraphicsStackItem, StackItem>::value, "Cannot add this class to a view : it's not a child class of GraphicsStackItem");
			static_assert(DrawableIsAuthorizedOnView<Drawable, ViewImpl>::value, "Unable to add this drawable : his type is not authorized to be attached on this kind of view.");
			assert(d != nullptr);
			auto& ref = *d.get();
			static_cast<View&>(*this).m_drawables->add(std::move(d));
			return ref;
		}
	};

	class DisplayWindow;
	class View : 
		public ViewAdder<View> {
		friend class ViewAdder<View>;

	public:
		virtual ~View() = default;

		View(View&&) noexcept = default;
		View& operator=(View&&) noexcept = default;
		View(const View&) = delete;
		View& operator=(const View&) = delete;

		auto getStackSize() const { return m_drawables->getSize(); }
		
		const DrawableStack& drawables() const { return *m_drawables;  }
		DrawableStack& drawables() { return *m_drawables; }

		void updateContentsOpacity(std::uint8_t opacity) { m_drawables->updateContentsOpacity(opacity); }

	protected:
		View() : m_drawables(std::make_unique<DrawableStack>()) {}
		void syncFromRawData(GraphicsContainer data, bool forceDetachAll = true) { m_drawables->syncFromRawData(std::move(data), forceDetachAll); }

	private:
		std::unique_ptr<DrawableStack> m_drawables;
	};
}

#endif