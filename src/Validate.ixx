module;
#include <optional>
#include <variant>
export module Validate;
import Types;
import Description;
export namespace Validate {
	using namespace Types;
	namespace L = Description::Lexer;
	namespace P = Description::Parser;
	namespace E = Description::Eval;

	using Generic = Symbol<Type::Function>::_Type;

	template <class T> struct Valid {
		static bool validate(Generic ast) { return Valid<E::Meaning<T>::what>::validate(ast);}
	};

	template <> struct Valid<E::None> {
		static bool validate(Generic ast) { return false; }
	};

	template <> struct Valid<L::Identifier> {
		static bool validate(Generic ast) {
			return std::holds_alternative<Symbol<Type::Identifier>>(ast);
		}
	};

	template <> struct Valid<L::Boolean> {
		static bool validate(Generic ast) {
			return std::holds_alternative<Symbol<Type::Boolean>>(ast);
		}
	};

	template <> struct Valid<L::Number> {
		static bool validate(Generic ast) {
			return std::holds_alternative<Symbol<Type::Number>>(ast);
		}
	};

	template <> struct Valid<P::List<>> {
		static bool validate(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::List>>(ast))
				return false;
			if (std::get<Symbol<Type::List>>(ast).value.empty()) return true;
			return false;
		}
	};

	template <class T, class... Ts> struct Valid<P::List<T, Ts...>> {
		static bool validate(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::List>>(ast))
				return false;
			auto l = std::get<Symbol<Type::List>>(ast);
			if (l.value.empty()) return false;
			auto fst = l.value.front();
			l.value.pop_front();
			return Valid<T>::validate(fst) && Valid<P::List<Ts...>>::validate(l);
		}
	};

	template <> struct Valid<E::Any> {
		static bool validate(Generic ast) { return true; }
	};

	template <class T> struct Valid<P::Repeat<T>> {
		static bool validate(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::List>>(ast))
				return false;
			auto l = std::get<Symbol<Type::List>>(ast);
			if (l.value.empty()) return false;
			return Valid<T>::validate(l.value.front());
		}
	};

	template <class T, class U> struct Valid<E::State<T, U>> {
		static bool validate(Generic ast) {
			if (!std::holds_alternative<Symbol<Type::List>>(ast))
				return false;
			auto l = std::get<Symbol<Type::List>>(ast);
			if (l.value.empty()) return false;
			if (Valid<T>::validate(l.value.front()) == false)
				return false;
			l.value.pop_front();
			return Valid<U>::validate(l);
		}
	};

	template <class... Ts> struct Valid<L::Either<Ts...>> {
		static bool validate(Generic ast) {
			return (Valid<Ts>::validate(ast) || ...);
		}
	};

}