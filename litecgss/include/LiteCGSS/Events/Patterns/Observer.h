#ifndef CGSS_OBSERVER_H
#define CGSS_OBSERVER_H

#include <functional>
#include <cassert>

namespace cgss {

	template <class T>
	class Observer {
	public:
		template <typename T1, template<typename T2, typename...> class Container, template <typename T2> class ObserverT>
		friend class Observable;

		explicit Observer(std::function<bool(T&)> handler) :
			m_handler(std::move(handler)) {
			assert(m_handler != nullptr && "Function cannot be null");
		}

		virtual ~Observer() = default;

	protected:
		// Called by an Observable this observer is still registered to, when
		// that Observable is destroyed: it must not be reached anymore.
		virtual void observableDestroyed() {}

	private:
		std::function<bool(T&)> m_handler;

		bool receive(T& evt) {
			return (m_handler)(evt);
		}

	};

}

#endif