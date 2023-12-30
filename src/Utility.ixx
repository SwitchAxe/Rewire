module;
#include <ranges>
export module Utility;

export namespace Utility {
    // thanks stackoverflow
    // https://stackoverflow.com/a/60971856
	namespace To {
        template <typename C>
        struct to_helper {};

        template <typename Container, std::ranges::range R>
            requires
        (std::convertible_to<std::ranges::range_value_t<R>,
            typename Container::value_type>)
        Container operator|(R&& r, to_helper<Container>) {
            return Container{ r.begin(), r.end() };
        }
        template <std::ranges::range Container>
            requires (!std::ranges::view<Container>)
        auto to() { return to_helper<Container>{}; }
	}

	namespace Meta {
		// finds a specified type among a series of arbitrary types.
		// yields true if it's found, false otherwise.
		template <class... Ts>
		struct Find { static constexpr bool value = false; };

		template <class T, class... Ts>
		requires (std::is_same_v<T, Ts> || ...)
		struct Find<T, Ts...> { static constexpr bool value = true; };
	}
}