module;
#include <vector>
#include <stack>
#include <map>
export module Globals;
import Types;
export namespace Globals {
	using Generic = Types::Symbol<Types::Type::Function>::_Type;
	std::stack<std::map<std::string, Generic>> identifiers;
	std::stack<std::map<std::string,
						std::pair<std::vector<std::string>,
								  Generic>>> user_procs;
	std::vector<std::string> function_cons;
}