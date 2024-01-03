module;
#include <string>
#include <list>
#include <variant>
export module Types;
import Utility;
export namespace Types {
	enum class Type {
		Number,
		String,
		Boolean,
		Function,
		Identifier,
		List
	};

	template <Type T>
	struct Symbol {};

	template <> struct Symbol<Type::Number> {
		Type type = Type::Number;
		unsigned long long int value = 0;
		bool is_root = false;

		auto eval() -> Symbol { return *this; }
	};

	template <> struct Symbol<Type::Identifier> {
		Type type = Type::Identifier;
		std::string value = "";
		bool is_root = false;

		auto eval() -> Symbol { return *this; }
	};
	
	template <> struct Symbol<Type::String> {
		Type type = Type::String;
		std::string value = "";
		bool is_root = false;

		auto eval() -> Symbol { return *this; }
	};

	template <> struct Symbol<Type::Boolean> {
		Type type = Type::Boolean;
		bool value = false;
		bool is_root = false;

		auto eval() -> Symbol { return *this; }
	};
	template <> struct Symbol<Type::Function>;
	// a list literal
	template <> struct Symbol<Type::List> {
		Type type = Type::List;
		std::list<std::variant<std::monostate,
								Symbol<Type::Number>,
								Symbol<Type::Identifier>,
								Symbol<Type::String>,
								Symbol<Type::Boolean>,
								Symbol<Type::List>,
								Symbol<Type::Function>>> value;
		auto eval() -> Symbol { return *this; }
	};
	// a function call
	template <> struct Symbol<Type::Function> {
		Type type = Type::Function;
		using _Type = std::variant<std::monostate,
								   Symbol<Type::Number>,
								   Symbol<Type::Identifier>,
								   Symbol<Type::String>,
								   Symbol<Type::Boolean>,
								   Symbol<Type::List>,
								   Symbol<Type::Function>>;
		std::list<_Type> value;

		auto eval() -> _Type;
	};
};