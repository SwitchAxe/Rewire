#include <algorithm>
#include <string>
export module Strings;
export namespace Strings {
	template <size_t N> struct String {
		char buf[N + 1] = {};
		constexpr String(const char (&s)[N]) {
			std::copy_n(s, N, buf);
		}
		auto operator<=>(const String&) const = default;
		constexpr operator std::string() const { return std::string{ buf }; }
	};
}