#ifndef CGSS_BOND_ELEMENT_H
#define CGSS_BOND_ELEMENT_H

#include "Bindable.h"

namespace cgss {
	template<class Value>
	class BondElement {
	private:
		Value m_value {};
		Bindable<Value>* m_target = nullptr;
	public:
		BondElement() = default;
		BondElement(const BondElement&) = delete;
		BondElement& operator=(const BondElement&) = delete;

		const Value& getValue() const { return m_value; };
		
		void setValue(Value value) {
			m_value = std::move(value);
			if (m_target != nullptr) {
				m_target->updateFromValue(&m_value);
			}
		}

		virtual ~BondElement() {
			if (m_target != nullptr) {
				m_target->bindValue(nullptr);
			}
		}

		void bind(Bindable<Value>* target) {
			if (target != m_target) {
				auto lastLinkedElement = m_target;
				m_target = nullptr;
				if (lastLinkedElement != nullptr) {
					lastLinkedElement->bindValue(nullptr);
				}
				m_target = target;
				if (m_target != nullptr) {
					m_target->bindValue(this);
				}
			}
		}

		template <class Function>
		void edit(Function&& operate) { 
			std::forward<Function>(operate)(m_value);
			if (m_target != nullptr) {
				m_target->updateFromValue(&m_value);
			}
		} 
	};
}

#endif
