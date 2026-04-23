#ifndef rbAdapter_H
#define rbAdapter_H
#include <string>
#include <stdexcept>
#include <functional>
#include "RubyValue.h"
#include <LiteCGSS/Common/Meta/metadata.h>
#include <type_traits>

extern VALUE rb_eRGSSError;

namespace rb
{
	namespace detail
	{
		template <class... Ts>
		struct voider
		{
			using type = void;
		};
	}

	template <class T, class = void>
	struct has_instance_method : std::false_type
	{
	};

	template <class T>
	struct has_instance_method<T, typename detail::voider<decltype(std::declval<T>().instance())>::type> : std::true_type
	{
	};

	using ErrorCallback = std::function<void(const std::string &)>;

	static inline bool CheckDisposedBool(VALUE self)
	{
		return RDATA(self)->data == nullptr;
	}

	template <class T, bool raise = true>
	std::string CheckDisposed(VALUE self)
	{
		if (CheckDisposedBool(self))
		{
			auto errorMessage = std::string{"Disposed "};
			errorMessage += meta::Log<T>::classname;
			errorMessage += ".";
			if constexpr (raise)
			{
				rb_raise(rb_eRGSSError, "%s", errorMessage.c_str());
			}
			return errorMessage;
		}
		return "";
	}

	template <class T>
	void Mark(void *data) {}

	template <class T>
	void Free(void *data)
	{
		delete reinterpret_cast<T *>(data);
	}

	template <class T>
	rb_data_type_t &GetDataType()
	{
		static rb_data_type_t type = {typeid(T).name(), {Mark<T>, Free<T>, nullptr, nullptr}, nullptr, nullptr, 0};
		return type;
	}

	template <class T>
	VALUE Alloc(VALUE klass)
	{
		return TypedData_Wrap_Struct(klass, &GetDataType<T>(), new T());
	}

	template <class T>
	auto *GetPtr(VALUE self)
	{
		T *ptr;
		TypedData_Get_Struct(self, T, &GetDataType<T>(), ptr);
		return ptr;
	}

	template <class T>
	auto &Get(VALUE self)
	{
		auto errorMessage = CheckDisposed<T>(self);
		if (!errorMessage.empty())
		{
			throw std::runtime_error(errorMessage);
		}
		return *GetPtr<T>(self);
	}

	static inline bool CheckTypeInvalidBool(VALUE self, VALUE expectedType)
	{
		return rb_obj_is_kind_of(self, expectedType) != Qtrue;
	}

	template <class T, bool raise = true>
	std::string CheckType(VALUE self, VALUE expectedType)
	{
		if (CheckTypeInvalidBool(self, expectedType))
		{
			auto errorMessage = std::string{"Expected "};
			errorMessage += meta::Log<T>::classname;
			errorMessage += " got ";
			errorMessage += RSTRING_PTR(rb_class_name(CLASS_OF(self)));
			errorMessage += ".";
			if constexpr (raise)
			{
				rb_raise(rb_eTypeError, "%s", errorMessage.c_str());
			}
			return errorMessage;
		}
		return "";
	}

	template <class T>
	T *GetSafeOrNull(VALUE self, VALUE expectedType)
	{
		if (CheckTypeInvalidBool(self, expectedType) ||
			CheckDisposedBool(self))
		{
			return nullptr;
		}

		auto *result = GetPtr<T>(self);
		if constexpr (has_instance_method<T>::value)
		{
			/* Here we ensure that if it is a LiteCGSS2 binding, it has been correctly initialized
			   (In order to avoid throwing C++ exception when accessing a badly initialized object) */
			if (result == nullptr || result->instance() == nullptr)
			{
				return nullptr;
			}
		}

		return result;
	}

	template <class T>
	VALUE RawDispose(VALUE self)
	{
		if (RDATA(self)->data == nullptr)
		{
			return Qnil;
		}
		delete GetPtr<T>(self);
		RDATA(self)->data = nullptr;
		return Qnil;
	}

	template <class T>
	VALUE Dispose(VALUE self)
	{
		// Do not soft crash if we try to dispose an already disposed object (dispose tolerance)
		if (RDATA(self)->data == nullptr)
		{
			return Qnil;
		}
		auto &element = rb::Get<T>(self);
		element->detach();
		return RawDispose<T>(self);
	}
}
#endif