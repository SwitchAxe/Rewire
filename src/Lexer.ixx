module;
#include <type_traits>
#include <vector>
#include <string>
#include <optional>
#include <iostream>
#include <ranges>
#include <string_view>
#include <cmath>
export module Lexer;
import Description;
import Strings;
export namespace Lexer {
	// PUNCTUATION TOKENS CANNOT BE PART OF IDENTIFIERS!!!!!
	using namespace Description::Lexer;

	namespace Split {
		template <class... Ts> struct Split {};

		template <char... Cs> struct Split<Punctuation<Cs>...> {
			static constexpr std::vector<std::string> split(std::string s) {
				return s |
					   std::views::split(std::string{ Cs... }) |
					   std::views::transform([](auto&& x) { return std::string_view{ x }; }) |
					   std::views::transform([](std::string_view sv) { return std::string{ sv }; }) |
					   std::ranges::to<std::vector<std::string>>();
			}
		};

		std::vector<std::string> split_text(std::string s) {
			std::vector<std::string> v = Split<LineContinuation, LineEndToken>::split(s);
			const auto ss = v | std::views::join | std::ranges::to<std::string>();
			return Split<LineEndToken>::split(ss);
		}
	}

	struct Token {
		std::string value;
	};

	// some utility stuff to check for
	// punctuation tokens.

	template <class T> struct Check {};
	template <char C> struct Check<Punctuation<C>> {
		std::string s;
		Check(std::string _s = "") : s(_s) {}
		size_t find(size_t off = 0) {
			size_t idx = s.find(C, off);
			return idx;
		}
	};

	template <char... Cs>
	struct Check<Either<Punctuation<Cs>...>> {
		std::string s;
		Check(std::string _s = "") : s(_s) {}
		size_t find(size_t off = 0) {
			return (size_t) std::min(std::initializer_list<size_t>{
				Check<Punctuation<Cs>>(s).find(off)...
			});
		}
	};

	// an overload to make stuff a little easier
	constexpr std::vector<std::string> operator+(std::vector<std::string> lhs,
										   std::vector<std::string> rhs) {
		lhs.insert(lhs.end(), rhs.begin(), rhs.end());
		return lhs;
	}


	// for debugging
	std::string print_tokens(std::vector<std::string> tks) {
		std::string s = "{ ";
		for (auto tk : tks) {
			s += "'" + tk + "' ";
		}
		s += "}";
		return s;
	}

	// the actual lexer starts here.

	template <class T> struct Lexer {};

	template <class T> struct Lexer<Any<T>> {
		static constexpr std::optional<std::vector<std::string>>
		lex(std::string s, size_t& si) {
			size_t cpy = si;
			std::optional<std::vector<std::string>> got = Lexer<T>::lex(s, si);
			if (got == std::nullopt) {
				si = cpy;
				return std::vector<std::string>{};
			}
			cpy = si;
			std::optional<std::vector<std::string>> next = Lexer<Any<T>>::lex(s, si);
			if (next == std::nullopt) {
				si = cpy;
				return got;
			}
			return *got + *next;
		}
	};

	// base case of the recursion
	template <> struct Lexer<Seq<>> {
		static constexpr std::optional<std::vector<std::string>>
		lex(std::string s, size_t& si) { return std::vector<std::string>{}; }
	};

	template <class T, class... Ts> struct Lexer<Seq<T, Ts...>> {
		static constexpr std::optional<std::vector<std::string>>
		lex(std::string s, size_t& si) {
			std::optional<std::vector<std::string>> result = Lexer<T>::lex(s, si);
			if (result == std::nullopt) return std::nullopt;
			std::optional<std::vector<std::string>> next = Lexer<Seq<Ts...>>::lex(s, si);
			if (next == std::nullopt) return std::nullopt;
			return *result + *next;
		}
	};

	template <> struct Lexer<Identifier> {
		static constexpr std::optional<std::vector<std::string>>
		lex(std::string s, size_t& si) {
			if (si >= s.length()) return std::nullopt;
			size_t off = Check<Punctuations>(s).find(si);
			if (off == si) return std::nullopt;
			std::string sub = s.substr(si, off - si);
			si += off - si;
			return { {sub} };
		}
	};

	template <> struct Lexer<String> {
		static constexpr std::optional<std::vector<std::string>>
		lex(std::string s, size_t& si) {
			if (si >= s.length()) return std::nullopt;
			if (s[si] != '"') return std::nullopt;
			size_t off = s.find('"', si);
			if (off == std::string::npos) return std::nullopt;
			si += off;
			std::string sub = s.substr(si, off);
			return { {sub} };
		}
	};

	template <> struct Lexer<Number> {
		static constexpr std::optional<std::vector<std::string>>
		lex(std::string s, size_t& si) {
			long long signed int n = 0;
			if (si >= s.length()) return std::nullopt;
			auto [ptr, ec] = std::from_chars(s.data() + si, s.data() + s.size(), n);
			if (ptr == s.data() + si) return std::nullopt;
			size_t len = n ? std::log10(std::abs(n)) + 1 : 1;
			std::string sub = s.substr(si, len);
			si += len;
			return { {sub} };
		}
	};

	template <> struct Lexer<Boolean> {
		static constexpr std::optional<std::vector<std::string>>
		lex(std::string s, size_t& si) {
			size_t start = si;
			if (si >= s.length()) return std::nullopt;
			si += 4;
			if (s.substr(start, 4) == "true") return { {"true"} };
			si += 1;
			if (s.substr(start, 5) == "false") return { {"false"} };
			return std::nullopt;
		}
	};

	template <char C> struct Lexer<Punctuation<C>> {
		static constexpr std::optional<std::vector<std::string>>
		lex(std::string s, size_t& si) {
			if (si >= s.length()) return std::nullopt;
			if (s[si] != C) return std::nullopt;
			si++;
			return { {std::string{C}} };
		}
	};

	template <Strings::String S> struct Lexer<Keyword<S>> {
		static constexpr std::optional<std::vector<std::string>>
		lex(std::string s, size_t& si) {
			size_t len = std::string{ S }.length();
			std::string sub = s.substr(si, len);
			si += len;
			if (sub == static_cast<std::string>(S)) return { {sub} };
			return std::nullopt;
		}
	};

	// base case
	template <> struct Lexer<Either<>> {
		static constexpr std::optional<std::vector<std::string>>
		lex(std::string s, size_t& si) { return std::nullopt; }
	};

	template <class T, class... Ts> struct Lexer<Either<T, Ts...>> {
		static constexpr std::optional<std::vector<std::string>>
		lex(std::string s, size_t& si) {
			std::optional<std::vector<std::string>> maybe;
			size_t cpy = si;
			maybe = Lexer<T>::lex(s, si);
			if (maybe == std::nullopt) return Lexer<Either<Ts...>>::lex(s, cpy);
			return maybe;
		}
	};

	// the actual interface to the lexer.

	template <class T> struct Tokens {};

	template <> struct Tokens<Either<>> {
		static constexpr std::optional<std::vector<std::string>>
		lex(std::string s) {
			return std::nullopt;
		}
	};

	template <class T, class... Ts> struct Tokens<Either<T, Ts...>> {
		static constexpr std::optional<std::vector<std::string>>
		lex(std::string s) {
			size_t start = 0;
			std::optional<std::vector<std::string>> maybe;
			maybe = Lexer<T>::lex(s, start);
			if (maybe == std::nullopt) return Tokens<Either<Ts...>>::lex(s);
			if (start < s.length()) return Tokens<Either<Ts...>>::lex(s);
			return maybe;
		}
	};

	// a function for convenience
	std::optional<std::vector<std::string>> tokenize(std::string s) {
		return Tokens<Forms>::lex(s);
	}


}