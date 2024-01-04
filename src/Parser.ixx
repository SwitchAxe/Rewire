module;
#include <string>
#include <optional>
#include <vector>
#include <typeinfo>
#include <variant>
#include <stdexcept>
#include <iostream>
export module Parser;
import Lexer;
import Description;
import Types;
export namespace Parser {
	// The parser is just a lexer with added functionality.
	// Instead of returning us a token stream, it returns us
	// the combined 'what' part of all our found types...
	// This way, we can then traverse this (also in the parser)
	// and translate it easily into a recursive data structure
	// fit for evaluation (in the visitor).
	using Generic = Types::Symbol<Types::Type::Function>::_Type;
	//forward declaration
	namespace L = Description::Lexer;
	namespace P = Description::Parser;

	template <class T> struct Parser {
		static std::optional<Generic>
		parse(std::vector<Lexer::Token> v, size_t& si) {
			if (si >= v.size()) return std::nullopt;
			return Parser<P::Describe<T>::what>::parse(v, si);
		}
	};

	template <> struct Parser<P::None> {
		static std::optional<Generic>
		parse(std::vector<Lexer::Token> v, size_t& si) {
			return std::nullopt;
		}
	};


	using namespace Types;

	// an overload to make working with lists easier
	Symbol<Type::List>
	operator+(Symbol<Type::List> lhs, Symbol<Type::List> rhs) {
		lhs.value.insert(lhs.value.end(), rhs.value.begin(), rhs.value.end());
		return lhs;
	}

	// a list builder overload
	Symbol<Type::List> operator+(Generic lhs, Generic rhs) {
		return Symbol<Type::List>{.value = { lhs, rhs }};
	}
	// aaaand to complete
	Symbol<Type::List> operator+(Symbol<Type::List> lhs, Generic rhs) {
		if (std::holds_alternative<Symbol<Type::List>>(rhs)) {
			auto l = std::get<Symbol<Type::List>>(rhs);
			l.value.insert(l.value.begin(), lhs.value.begin(), lhs.value.end());
			return l;
		}
		lhs.value.push_back(rhs);
		return lhs;
	}
	Symbol<Type::List> operator+(Generic lhs, Symbol<Type::List> rhs) {
		if (std::holds_alternative<Symbol<Type::List>>(lhs)) {
			auto l = std::get<Symbol<Type::List>>(lhs);
			l.value.insert(l.value.end(), rhs.value.begin(), rhs.value.end());
			return l;
		}
		rhs.value.push_front(lhs);
		return rhs;
	}

	// base case
	template <> struct Parser<P::List<>> {
		static std::optional<Symbol<Type::List>>
		parse(std::vector<Lexer::Token> v, size_t& si) {
			return Symbol<Type::List>{};
		}
	};


	template <class T, class... Ts> struct Parser<P::List<T, Ts...>> {
		static std::optional<Symbol<Type::List>>
		parse(std::vector<Lexer::Token> v, size_t& si) {
			if (si >= v.size()) return std::nullopt;
			auto got = Parser<T>::parse(v, si);
			if (got == std::nullopt) return std::nullopt;
			auto others = Parser<P::List<Ts...>>::parse(v, si);
			if (others == std::nullopt) return std::nullopt;
			return (*got) + (*others);
		}
	};

	template <class... Ts, class... Us>
	struct Parser<P::List<P::List<Ts...>, Us...>> {
		static std::optional<Symbol<Type::List>>
		parse(std::vector<Lexer::Token> v, size_t& si) {
			return Parser<P::List<Ts..., Us...>>::parse(v, si);
		}
	};

	template <class T, class... Us>
	struct Parser<P::List<P::Repeat<T>, Us...>> {
		static std::optional<Symbol<Type::List>>
		parse(std::vector<Lexer::Token> v, size_t& si) {
			auto first_half = Parser<P::Repeat<T>>::parse(v, si);
			if (first_half == std::nullopt) return std::nullopt;
			auto second_half = Parser<P::List<Us...>>::parse(v, si);
			if (second_half == std::nullopt) return std::nullopt;
			return (*first_half) + (*second_half);
		}
	};

	template <class T, class... Ts> struct Parser<P::List<P::Ignore<T>, Ts...>> {
		static std::optional<Symbol<Type::List>>
		parse(std::vector<Lexer::Token> v, size_t& si) {
			if (si >= v.size()) return std::nullopt;
			// Parse, but ignore the result
			auto unused = Parser<T>::parse(v, si);
			if (unused == std::nullopt) return std::nullopt;
			return Parser<P::List<Ts...>>::parse(v, si);
		}
	};


	template <class T, class... Ts> struct Parser<P::List<P::Optional<T>, Ts...>> {
		static std::optional<Symbol<Type::List>>
		parse(std::vector<Lexer::Token> v, size_t& si) {
			if (si >= v.size()) return std::nullopt;
			// silently move on if Optional<T> fails to parse.
			size_t si_cpy = si;
			auto maybe = Parser<T>::parse(v, si);
			if (maybe != std::nullopt) {
				auto others = Parser<P::List<Ts...>>::parse(v, si);
				if (others == std::nullopt) return std::nullopt;
				return (*maybe) + (*others);
			}
			si = si_cpy;
			return Parser<P::List<Ts...>>::parse(v, si);
		}
	};

	template <class T> struct Parser<P::Repeat<T>> {
		static std::optional<Symbol<Type::List>>
		parse(std::vector<Lexer::Token> v, size_t& si) {
			// a Repeat parses pretty much the same as a list:
			Symbol<Type::List> ret;
			if (si >= v.size()) return std::nullopt;
			size_t si_cpy = si;
			auto got = Parser<T>::parse(v, si);
			if (got == std::nullopt) { si = si_cpy; return std::nullopt; }
			ret = ret + *got;
			if (si >= v.size()) return ret;
			auto others = Parser<P::Repeat<T>>::parse(v, si);
			if (others == std::nullopt) { return ret; }
			return ret + *others;
		}
	};


	template <class T> struct Charset {};
	template <> struct Charset<L::Either<>> {
		static constexpr bool contains(char c) { return false; }
	};
	template <char C, char... Cs> struct Charset<L::Either<L::Punctuation<C>, L::Punctuation<Cs>...>> {
		static constexpr bool contains(char c) {
			return (c == C) || Charset<L::Either<L::Punctuation<Cs>...>>::contains(c);
		}
	};

	template <> struct Parser<P::Identifier> {
		static std::optional<Generic>
			parse(std::vector<Lexer::Token> v, size_t& si) {
			if (si >= v.size()) return std::nullopt;
			std::string s = v[si].value;
			si++;
			return Symbol<Type::Identifier>{.value = s};
		}
	};

	template <> struct Parser<L::Not<L::Punctuations>> {
		static std::optional<Generic>
		parse(std::vector<Lexer::Token> v, size_t& si) {
			if (v[si].value.length() > 1)
				return Parser<P::Identifier>::parse(v, si);
			if (Charset<L::Punctuations>::contains(v[si].value[0]))
				return std::nullopt;
			return Parser<P::Identifier>::parse(v, si);
		}
	};


	template <char C> struct Parser<L::Punctuation<C>> {
		static std::optional<Generic>
			parse(std::vector<Lexer::Token> v, size_t& si) {
			if (v[si].value != std::string{ C }) return std::nullopt;
			return Parser<P::Identifier>::parse(v, si);
		}
	};

	template <> struct Parser<P::Literal> {
		static std::optional<Generic>
		parse(std::vector<Lexer::Token> v, size_t& si) {
			si++;
			return Symbol<Type::String>{.value = v[si].value};
		}
	};

	template <> struct Parser<L::Either<>> {
		static std::optional<Generic>
		parse(std::vector<Lexer::Token>, size_t&) {
			return std::nullopt;
		}
	};

	template <class T, class... Ts> struct Parser<L::Either<T, Ts...>> {
		static std::optional<Generic>
		parse(std::vector<Lexer::Token> v, size_t& si) {
			size_t si_cpy = si;
			auto ret = Parser<T>::parse(v, si);
			if (ret != std::nullopt) return ret;
			si = si_cpy;
			return Parser<L::Either<Ts...>>::parse(v, si);
		}
	};

	std::optional<Generic> get_ast(std::vector<Lexer::Token> v) {
		size_t start = 0;
		return Parser<L::Statement>::parse(v, start);
	}


	std::string print_ast(Generic ast) {
		if (std::holds_alternative<Symbol<Type::Identifier>>(ast))
			return "(ID) " + std::get<Symbol<Type::Identifier>>(ast).value + " ";
		if (std::holds_alternative<Symbol<Type::String>>(ast))
			return "(STR) " + std::get<Symbol<Type::String>>(ast).value + " ";
		if (std::holds_alternative<Symbol<Type::Number>>(ast))
			return "(NUM) " + std::to_string(std::get<Symbol<Type::Number>>(ast).value) + " ";
		if (std::holds_alternative<Symbol<Type::Boolean>>(ast))
			return "(BOOL) " + std::to_string(std::get<Symbol<Type::Boolean>>(ast).value) + " ";
		if (std::holds_alternative<Symbol<Type::List>>(ast)) {
			std::string ret = "(LIST) { ";
			for (auto x : std::get<Symbol<Type::List>>(ast).value) {
				ret += print_ast(x);
			}
			ret += "}";
			return ret;
		}
		return "";
	}

}