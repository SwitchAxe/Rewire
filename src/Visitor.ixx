module;
#include <variant>
#include <stack>
#include <utility>
#include <map>
#include <optional>
#include <iostream>
export module Visitor;
import Types;
import Description;
import Parser;
export namespace Visitor{
	using Generic = Types::Symbol<Types::Type::Function>::_Type;
	using namespace Description::Eval;
	namespace L = Description::Lexer;
	namespace P = Description::Parser;
	std::stack<std::map<std::string, Generic>> identifiers;

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

	template <class T> struct Charset {};
	template <> struct Charset<L::Either<>> {
		static constexpr bool contains(char c) { return false; }
	};
	template <char... Cs> struct Charset<L::Either<L::Punctuation<Cs>...>> {
		static constexpr bool contains(char c) {
			return ((c == Cs) || ...);
		}
	};

	template <class T> struct Visitor {
		static std::optional<Generic> visit(Generic ast) {
			return Visitor<T::what>::visit(ast);
		}
	};

	template <class T> struct Visitor<Repeat<T>> {
		static std::optional<Generic> visit(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::List>>(ast))
				return std::nullopt;
			auto l = std::get<Symbol<Type::List>>(ast);
			if (l.value.empty()) return Symbol<Type::List>{};
			auto elem = Visitor<T>::visit(l.value.front());
			l.value.pop_front();
			if (elem == std::nullopt) return std::nullopt;
			auto others = Visitor<Repeat<T>>::visit(l);
			if (others == std::nullopt) return std::nullopt;
			return (*elem) + (std::get<Symbol<Type::List>>(*others));
		}
	};

	template <> struct Visitor<Identifier> {
		static std::optional<Generic> visit(Generic ast) {
			if (std::holds_alternative<Symbol<Type::Identifier>>(ast))
				return ast;
			return std::nullopt;
		}
	};

	template <> struct Visitor<Description::Eval::Any> {
		static std::optional<Generic> visit(Generic ast) {
			// check if we have a punctuation token...
			if (!std::holds_alternative<Symbol<Type::Identifier>>(ast))
				return ast;
			auto id = std::get<Symbol<Type::Identifier>>(ast);
			if (id.value.size() != 1) return ast;
			if (Charset<L::Punctuations>::contains(id.value[0]))
				return std::nullopt;
			return ast;
		}
	};

	template <class T, class U> struct Visitor<State<T, U>> {
		static std::optional<Generic> visit(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::List>>(ast))
				return std::nullopt;
			auto l = std::get<Symbol<Type::List>>(ast);
			auto id = Visitor<T>::visit(l.value.front());
			l.value.pop_front();
			if (id == std::nullopt) return std::nullopt;
			auto any = Visitor<U>::visit(l);
			if (any == std::nullopt) return std::nullopt;
			auto call = std::visit([]<class T, class U>(T t, U u)
							-> Symbol<Type::Function> {
								auto l = t + u;
								return Symbol<Type::Function>{
									.value = l.value
								};
							}, *id, *any);
			return call.eval();
		}
	};



	// base case
	template <> struct Visitor<Either<>> {
		static std::optional<Generic> visit(Generic ast) {
			return std::nullopt;
		}
	};

	template <class T, class... Ts> struct Visitor<Either<T, Ts...>> {
		static std::optional<Generic> visit(Generic ast) {
			std::optional<Generic> maybe = Visitor<T>::visit(ast);
			if (maybe == std::nullopt) return Visitor<Either<Ts...>>::visit(ast);
			return *maybe;
		}
	};

	std::optional<Generic> evaluate(Generic ast) {
		return Visitor<Description::Eval::ToEval>::visit(ast);
	}


}