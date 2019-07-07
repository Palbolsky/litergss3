#ifndef LR_VIEW_H
#define LR_VIEW_H
#include <cassert>
#include <memory>
#include <vector>

#include "Drawable.h"
#include "DrawableStack.h"

namespace lr {

	//We only expose the "add" method to the internal of Drawable class
	template <class View>
	class ViewAdder {
		template <class T>
		friend class Drawable;
	private:
		template <class DrawableChild>
		DrawableChild& add(std::unique_ptr<DrawableChild> d) {
			assert(d != nullptr);
			auto& ref = *d.get();
			static_cast<View&>(*this).m_drawables.add(std::move(d));
			return ref;
		}
	};

	class View : 
		public ViewAdder<View> {
		friend class ViewAdder<View>;
	public:
		View() = default;
		virtual ~View() = default;

		auto getSize() { return m_drawables.getSize(); }
		void draw(sf::View& view, sf::RenderTarget& target) const { m_drawables.draw(view, target); }

	private:
		DrawableStack m_drawables;
	};
}

#endif