#ifndef CGSS_SUBOBSERVER_H
#define CGSS_SUBOBSERVER_H
#include "LiteCGSS/Events/Patterns/Observer.h"

namespace cgss {
	template <class T, template<typename T1, typename...> class Container, template <typename T1> class ObserverT>
	class Observable;

	template <class T, template<typename T1, typename...> class Container = std::vector, template <typename T2> class ObserverT = Observer>
	class SubObserver :
		public ObserverT<T> {
			using ObservableType = Observable<T, Container, ObserverT>;
	public:
		template <class ... Args>
		explicit SubObserver(std::function<bool(T&)> const& handler, ObservableType& observable, Args&& ... args) :
			ObserverT<T>(std::forward<Args>(args)..., handler),
			m_observable(&observable) {
			m_observable->ObservableType::addObserver(static_cast<ObserverT<T>&>(*this));
		}

		virtual ~SubObserver() {
			if (m_observable != nullptr) {
				m_observable->ObservableType::removeObserver(static_cast<ObserverT<T>&>(*this));
			}
		}

	protected:
		void observableDestroyed() override {
			m_observable = nullptr;
		}

	private:
		// Null once the observable is destroyed (see Observable::~Observable):
		// a sub-observer may outlive it, e.g. a viewport outliving its window.
		ObservableType* m_observable;
	};
}
#endif
