module;
#include <functional>
#include <string>
#include <stdexcept>
#include <variant>
#include <ranges>
#include <optional>
export module Functions;
import Globals;
import Types;
import Builtins;
import Parser;
export namespace Functions {
	using Generic = Types::Symbol<Types::Type::Function>::_Type;
	std::optional<Generic> evaluate(Generic root);
	// for pattern matching
	template<class... Ts>
	struct overloaded : Ts... { using Ts::operator()...; };
	using namespace Types;
	// to call USER DEFINED procedures
	struct Call {
		Generic body;
		std::list<Generic> args;
		std::vector<std::string> params;
		std::string name;
		Call(std::string _n, std::list<Generic> _args) {
			if (!Globals::user_procs.top().contains(_n))
				throw std::logic_error{ "Undefined procedure <" + _n + ">\n" };
			auto current = Globals::user_procs.top();
			name = _n;
			body = current[_n].second;
			params = current[_n].first;
			args = _args;
		}

		Generic eval_parameters(Generic root, std::string param, Generic arg) {
			using namespace Types;
			return std::visit(overloaded {
					[&](Symbol<Type::List> x) -> Generic
					{ return Symbol<Type::List>{
						.value = x.value |
								 std::views::transform([&](auto x) -> Generic
								 { return eval_parameters(x, param, arg); }) |
								 std::ranges::to<std::list<Generic>>()
						}; },
					[&](Symbol<Type::Identifier> x) -> Generic
					{ if (x.value == param) return arg; return x; },
					[](auto else_clause) -> Generic
					{ return else_clause; } }, root);
		}

		Generic call() {
			if (params.size() != args.size())
				throw std::logic_error{ "Called the function <" + name + ">"
										" with a wrong number of arguments! "
										"(expected " +
										std::to_string(params.size()) +
										" arguments and got " +
										std::to_string(args.size()) +
										" instead)\n" };
			for (auto s : params) {
				body = eval_parameters(body, s, args.front());
				args.pop_front();
			}

			auto ret = evaluate(body);
			if (ret != std::nullopt) return *ret;
			throw std::logic_error{ "Error while calling <" + name +
								    "> with arguments <" +
								    Parser::print_ast(
										Symbol<Type::List>{.value = args}) +
									">!\n"};
		}

	};
	// Enable == false: builtin function
	template <bool Enable>
	struct Functor {
		static Generic call(std::string p, Symbol<Type::List> args) {
			if (Builtins::procedures.contains(p)) {
				return
					Builtins::procedures[p](args);
			}
			throw std::logic_error{ "Undefined function <" + p + ">\n" };
		}
	};

	template <>
	struct Functor<true> {
		static Generic call(std::string p, std::list<Generic> args) {
			return Call(p, args).call();
		}
	};
}