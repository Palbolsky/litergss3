#ifndef CGSS_BINDABLE_H
#define CGSS_BINDABLE_H

namespace cgss {
    template<class> class BondElement;

    template<class Value>
    class Bindable {
        friend class BondElement<Value>;
    public:
        Bindable() = default;
        Bindable(const Bindable&) = delete;
        Bindable& operator=(const Bindable&) = delete;
        
        virtual ~Bindable() {
            m_destructing = true;
            if (m_target != nullptr) {
                m_target->bind(nullptr);
            }
        }

        void bindValue(BondElement<Value>* element) {
            if (element != m_target) {
                auto lastLinkedValue = m_target;
                m_target = nullptr;
                if (lastLinkedValue != nullptr) {
                    lastLinkedValue->bind(nullptr);
                }
                m_target = element;
                if (m_target != nullptr) {
                    m_target->bind(this);
                    if (!m_destructing) {
                        updateFromValue(&m_target->getValue());
                    }
                } else if (!m_destructing) {
                    updateFromValue(nullptr);
                }
            }
        }

    protected:
        virtual void updateFromValue(const Value* value) = 0;

    private:
        BondElement<Value>* m_target = nullptr;
        bool m_destructing = false;
    };
}

#endif