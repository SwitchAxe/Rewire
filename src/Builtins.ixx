module;
#include <map>
#include <functional>
#include <string>
#include <variant>
#include <ranges>
#include <stdexcept>
import Types;
import Utility;
export module Builtins;
export namespace Builtins {
	// a few basic operators
	constexpr Types::Symbol<Types::Type::Number>
	operator+(Types::Symbol<Types::Type::Number> lhs,
			  Types::Symbol<Types::Type::Number> rhs) {
		return Types::Symbol<Types::Type::Number>
				{ .value = lhs.value + rhs.value,
				  .is_root = false };
	}

	constexpr Types::Symbol<Types::Type::Number>
		operator-(Types::Symbol<Types::Type::Number> lhs,
			Types::Symbol<Types::Type::Number> rhs) {
		return Types::Symbol<Types::Type::Number>
				{.value = lhs.value - rhs.value,
				 .is_root = false };
	}

	constexpr Types::Symbol<Types::Type::Number>
		operator*(Types::Symbol<Types::Type::Number> lhs,
			Types::Symbol<Types::Type::Number> rhs) {
		return Types::Symbol<Types::Type::Number>
				{.value = lhs.value * rhs.value,
				 .is_root = false };
	}

	constexpr Types::Symbol<Types::Type::Number>
		operator/(Types::Symbol<Types::Type::Number> lhs,
			Types::Symbol<Types::Type::Number> rhs) {
		return Types::Symbol<Types::Type::Number>
				{.value = lhs.value / rhs.value,
				 .is_root = false };
	}

	struct Functions {
		using Func = Types::Symbol<Types::Type::Function>;
		static Func::_Type
		first(Types::Symbol<Types::Type::List> l) {
			return l.value.front();
		}
		
		static Func::_Type
		rest(Types::Symbol<Types::Type::List> l) {
			l.value.pop_front();
			return l;
		}
	};
	using Func = Types::Symbol<Types::Type::Function>;
	using Signature =
		std::function<Func::_Type(Types::Symbol<Types::Type::List>)>;
	std::map<std::string, Signature> procedures = {
		{"first", Functions::first},
		{"rest", Functions::rest},
	};

	// Call a builtin function (for user-defined functions, use
	// the 'Invoke' namespace (NOT YET DEFINED, TODO)
	namespace Call {
		template <class C> struct Helper {};
		using namespace Types;
		using _Type = std::variant<Symbol<Type::Number>,
								   Symbol<Type::Identifier>,
								   Symbol<Type::Boolean>,
								   Symbol<Type::List>,
								   Symbol<Type::String>,
								   Symbol<Type::Function>>;
		Symbol<Type::Function>::_Type call(Symbol<Type::Function> f) {
			auto ident = std::get<Symbol<Type::Identifier>>(f.value.front());
			std::string name = ident.value;
			f.value.pop_front();
			// create a List Literal with the contents of the function call
			// f:
			Symbol<Type::List> l = {.value = f.value};
			if (name == "first") {
				return Functions::first(l);
			}
			throw std::logic_error{"placeholder!\n"};
		}
	}
}

// temporary location for the eval() of function Symbols
using namespace Types;
Symbol<Type::Function>::_Type Symbol<Type::Function>::eval() {
	using T = Symbol<Type::Function>::_Type;
	return
		Builtins::Call::call(Symbol<Type::Function>
							  {.value =
								 this->value |
								 std::ranges::views::transform(
									 [](auto x) -> T
									   { return
											std::visit([](auto y) -> T
												{ return y.eval(); },
												x); }) |
								 Utility::To::to<decltype(this->value)>()}); }