// Rewire.cpp : Questo file contiene la funzione 'main', in cui inizia e termina l'esecuzione del programma.
//

#include <iostream>
#include <vector>
#include <optional>
import Lexer;
import Parser;
import Visitor;
import Types;
using Generic = Types::Symbol<Types::Type::Function>::_Type;
int main() {
    std::optional<std::vector<Lexer::Token>> tks =
		Lexer::tokenize("(plus 1 2 3 69)");
	Lexer::print_tokens("Final Tokens:", *tks);
	try {
		auto ast = Parser::get_ast(*tks);
		if (ast == std::nullopt)
			throw std::logic_error{ "Didn't find an ast!\n" };
		std::cout << Parser::print_ast(*ast) << "\n";
		std::cout << "visiting...\n";
		auto visited = Visitor::evaluate(*ast);
		if (visited == std::nullopt)
			throw std::logic_error{ "Failed!\n" };
		std::cout << Parser::print_ast(*visited) << "\n";
	} catch (std::logic_error ex) {
		std::cout << ex.what();
	}
	return 0;
}