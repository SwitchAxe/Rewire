module;
#include <map>
#include <functional>
#include <string>
#include <variant>
#include <ranges>
#include <stdexcept>
#include <numeric>
#include <optional>
export module Builtins;
import Types;
import Globals;
import Parser;
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

		static Func::_Type
		plus(Types::Symbol<Types::Type::List> l) {
			signed long long int init = 0;
			for (Func::_Type x : l.value) {
				Types::Symbol<Types::Type::Number> e =
					std::get<Types::Symbol<Types::Type::Number>>(x);
				init += e.value;
			}
			return Types::Symbol<Types::Type::Number> {.value = init};
		}

	};
	using Func = Types::Symbol<Types::Type::Function>;
	using Signature =
		std::function<Func::_Type(Types::Symbol<Types::Type::List>)>;
	std::map<std::string, Signature> procedures = {
		{"first", Functions::first},
		{"rest", Functions::rest},
		{"plus", Functions::plus}
	};
}