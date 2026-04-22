#ifndef CGSS_BACKEND_CONCEPT_H
#define CGSS_BACKEND_CONCEPT_H

#include <type_traits>

namespace cgss::backend {

	namespace detail {
		template <typename, typename = void>
		struct has_backend_typedefs : std::false_type {};

		template <typename T>
		struct has_backend_typedefs<T, std::void_t<
			typename T::native_render_target,
			typename T::native_texture,
			typename T::native_image,
			typename T::native_sprite,
			typename T::native_view,
			typename T::native_render_states,
			typename T::native_font,
			typename T::native_event,
			typename T::native_window,
			typename T::native_render_texture,
			typename T::native_shader,
			typename T::native_vertex_array
		>> : std::true_type {};

		template <typename, typename = void>
		struct has_capability_tags : std::false_type {};

		template <typename T>
		struct has_capability_tags<T, std::void_t<
			decltype(T::has_window),
			decltype(T::has_event_pump),
			decltype(T::owns_gpu_memory)
		>> : std::true_type {};
	}

	template <typename T>
	constexpr bool is_backend_v =
		detail::has_backend_typedefs<T>::value &&
		detail::has_capability_tags<T>::value;

#ifdef __cpp_concepts
	template <typename T>
	concept Backend = requires {
		typename T::native_render_target;
		typename T::native_texture;
		typename T::native_image;
		typename T::native_sprite;
		typename T::native_view;
		typename T::native_render_states;
		typename T::native_font;
		typename T::native_event;
		typename T::native_window;
		typename T::native_render_texture;
		typename T::native_shader;
		typename T::native_vertex_array;
		{ T::has_window }      -> std::convertible_to<bool>;
		{ T::has_event_pump }  -> std::convertible_to<bool>;
		{ T::owns_gpu_memory } -> std::convertible_to<bool>;
	};
#endif

}

#endif
