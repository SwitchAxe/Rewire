module;
#include <variant>
#include <stack>
#include <utility>
#include <map>
#include <optional>
#include <iostream>
#include <vector>
#include <string>
export module Visitor;
import Types;
import Description;
import Parser;
import Validate;
import Strings;
import Globals;
import Functions;
import Builtins;
export namespace Visitor {
	using namespace Description::Eval;
	using namespace Globals;
	namespace L = Description::Lexer;
	namespace P = Description::Parser;
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
	template <char... Cs> struct Charset<L::Either<L::Punctuation<Cs>...>> {
		static constexpr bool contains(char c) {
			return ((c == Cs) || ...);
		}
	};

	template <class T> struct Words {};

	template <Strings::String... Ss> struct Words<L::Either<Keyword<Ss>...>> {
		static constexpr bool contains(std::string s) {
			return ((s == Ss) || ...);
		}
	};


	template <class T> struct Visitor {
		static std::optional<Generic> visit(Generic ast) {
			return Visitor<Meaning<T>::what>::visit(ast);
		}
	};

	// ATOMIC TYPES (basic types)

	template <Strings::String S> struct Visitor<L::Keyword<S>> {
		static std::optional<Generic> visit(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::Identifier>>(ast))
				return std::nullopt;
			auto s = std::get<Symbol<Type::Identifier>>(ast);
			if (s.value == static_cast<std::string>(S))
				return s;
			return std::nullopt;
		}
	};

	template <char C> struct Visitor<L::Punctuation<C>> {
		static std::optional<Generic> visit(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::Identifier>>(ast))
				return std::nullopt;
			auto s = std::get<Symbol<Type::Identifier>>(ast);
			if (s.value.size() != 1) return std::nullopt;
			if (s.value[0] == C) return s;
			return std::nullopt;
		}
	};

	template <> struct Visitor<L::Identifier> {
		static std::optional<Generic> visit(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::Identifier>>(ast))
				return std::nullopt;

			auto s = std::get<Symbol<Type::Identifier>>(ast);
			if (Words<L::Keywords>::contains(s.value))
				return std::nullopt;
			if (s.value.size() == 1)
				if (Charset<L::Punctuations>::contains(s.value[0]))
					return std::nullopt;
			return ast;
		}
	};


	template <> struct Visitor<L::Number> {
		static std::optional<Generic> visit(Generic ast) {
			if (std::holds_alternative<Symbol<Type::Number>>(ast))
				return ast;
			return std::nullopt;
		}
	};

	template <> struct Visitor<L::Boolean> {
		static std::optional<Generic> visit(Generic ast) {
			if (std::holds_alternative<Symbol<Type::Boolean>>(ast))
				return ast;
			return std::nullopt;
		}
	};

	template <> struct Visitor<L::String> {
		static std::optional<Generic> visit(Generic ast) {
			if (std::holds_alternative<Symbol<Type::String>>(ast))
				return ast;
			return std::nullopt;
		}
	};

	template <class T, class U> struct Visitor<State<T, U>> {
		static std::optional<Generic> visit(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::List>>(ast))
				return std::nullopt;
			auto l = std::get<Symbol<Type::List>>(ast);
			auto id = Visitor<T>::visit(l.value.front());
			if (id == std::nullopt) return std::nullopt;
			l.value.pop_front();
			auto any = Visitor<U>::visit(l);
			if (any == std::nullopt) return std::nullopt;
			// todo allow for 'any' to be more than just
			// a list. should it be possible to
			// make the user define functions that
			// act on single numbers directly, for instance?
			std::string name = std::get<Symbol<Type::Identifier>>(*id).value;
			auto args = std::get<Symbol<Type::List>>(*any);
			if (Builtins::procedures.contains(name))
				return Functions::Functor<false>::call(name, args);
			return Functions::Functor<true>::call(name, args.value);
		}
	};


	template <> struct Visitor<Description::Eval::Any> {
		static std::optional<Generic> visit(Generic ast) {
			// check if we have a punctuation token...
			if (!std::holds_alternative<Symbol<Type::Identifier>>(ast))
				return Functions::evaluate(ast);
			auto id = std::get<Symbol<Type::Identifier>>(ast);
			if (id.value.size() != 1) return ast;
			if (Charset<L::Punctuations>::contains(id.value[0]))
				return std::nullopt;
			return Functions::evaluate(ast);
		}
	};

	template <class T>
	struct Visitor<NoEval<T>> {
		static std::optional<Generic> visit(Generic ast) {
			auto check = Validate::Valid<T>::validate(ast);
			if (check) return ast;
			return std::nullopt;
		}
	};

	template <> struct Visitor<Description::Eval::None> {
		static std::optional<Generic> visit(Generic ast) {
			return std::nullopt;
		}
	};

	// COMPOUND TYPES (it's useful to think of them as iterative types/blocks)

	template <class T> struct Visitor<Repeat<T>> {
		static std::optional<Generic> visit(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::List>>(ast))
				return std::nullopt;
			auto l = std::get<Symbol<Type::List>>(ast);
			if (l.value.empty()) return Symbol<Type::List>{};
			auto elem = Visitor<T>::visit(l.value.front());
			if (elem == std::nullopt) return std::nullopt;
			l.value.pop_front();
			auto others = Visitor<Repeat<T>>::visit(l);
			if (others == std::nullopt) return Symbol<Type::List>{.value = { *elem } };
			return (*elem) + (std::get<Symbol<Type::List>>(*others));
		}
	};

	// base case
	template <> struct Visitor<List<>> {
		static std::optional<Generic> visit(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::List>>(ast))
				return std::nullopt;
			if (auto l = std::get<Symbol<Type::List>>(ast); l.value.empty())
				return l;
			return std::nullopt;
		}
	};

	template <class T, class... Ts> struct Visitor<List<T, Ts...>> {
		static std::optional<Symbol<Type::List>> visit(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::List>>(ast))
				return std::nullopt;
			auto l = std::get<Symbol<Type::List>>(ast);
			auto got = Visitor<T>::visit(l.value.front());
			if (got == std::nullopt) return std::nullopt;
			if (l.value.empty()) return std::nullopt;
			l.value.pop_front();
			auto others = Visitor<List<Ts...>>::visit(l);
			if (others == std::nullopt) return std::nullopt;
			return (*got) + (*others);
		}
	};

	template <class T, class... Ts> struct Visitor<List<Repeat<T>, Ts...>> {
		static std::optional<Symbol<Type::List>> visit(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::List>>(ast))
				return std::nullopt;
			auto l = std::get<Symbol<Type::List>>(ast);
			if (l.value.empty()) return std::nullopt;
			auto got = Visitor<Repeat<T>>::visit(l);
			if (got == std::nullopt) return std::nullopt;
			auto r = std::get<Symbol<Type::List>>(*got);
			size_t cnt = 0;
			while (cnt < r.value.size()) { l.value.pop_front(); cnt++; }
			auto others = Visitor<List<Ts...>>::visit(l);
			if (others == std::nullopt) return std::nullopt;
			return (*got) + (*others);
		}
	};

	template <class T, class... Ts> struct Visitor<List<Ignore<T>, Ts...>> {
		static std::optional<Generic> visit(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::List>>(ast))
				return std::nullopt;
			auto l = std::get<Symbol<Type::List>>(ast);
			if (l.value.empty()) return std::nullopt;
			auto sym = l.value.front();
			if (Visitor<T>::visit(sym) == std::nullopt) return std::nullopt;
			l.value.pop_front();
			auto rest = Visitor<List<Ts...>>::visit(l);
			return rest;
		}
	};

	// NOT a base case.
	template <> struct Visitor<Let<>> {
		static std::optional<Generic> visit(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::Identifier>>(ast))
				return std::nullopt;
			auto s = std::get<Symbol<Type::Identifier>>(ast).value;
			if (Words<Keywords>::contains(s)) return std::nullopt;
			if (s.length() == 1)
				if (Charset<Punctuations>::contains(s[0]))
					return std::nullopt;
			function_cons.push_back(s);
			return Symbol<Type::Boolean>{.value = true};
		}
	};

	template <class... Ts> struct Visitor<List<Let<>, Ts...>> {
		static std::optional<Generic> visit(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::List>>(ast))
				return std::nullopt;
			auto l = std::get<Symbol<Type::List>>(ast);
			if (l.value.empty()) return std::nullopt;
			Visitor<Let<>>::visit(l.value.front());
			l.value.pop_front();
			return Visitor<List<Ts...>>::visit(l);
		}
	};

	template <class... Ts> struct Visitor<List<LetEnd, Ts...>> {
		static std::optional<Generic> visit(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::List>>(ast))
				return std::nullopt;
			auto l = std::get<Symbol<Type::List>>(ast);
			std::optional<Generic> body;
			if (l.value.size() > 1) body = Visitor<NoEval<List<Ts...>>>::visit(l);
			else if constexpr (sizeof...(Ts) != 1) return std::nullopt;
			else body = Visitor<NoEval<Ts...>>::visit(l.value.front());
			if (function_cons.empty()) return std::nullopt;
			auto name = function_cons[0];
			function_cons.erase(function_cons.begin());
			if (user_procs.empty()) user_procs.push({});
			auto top = user_procs.top();
			user_procs.pop();

			top.insert_or_assign(name, std::make_pair(function_cons, *body ));
			user_procs.push(top);
			return Symbol<Type::Boolean>{.value = true};
		}
	};

	template <class... Ts>
	struct Visitor<Let<Ts...>> {
		static std::optional<Generic> visit(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::List>>(ast))
				return std::nullopt;
			return Visitor<List<Ts...>>::visit(ast);
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

namespace Functions {
	std::optional<Generic> evaluate(Generic ast) {
		return Visitor::evaluate(ast);
	}
}