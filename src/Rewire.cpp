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
	std::string input = "(plus 1 2 3) | (plus 3 4)\n(plus 1 2)";
	for (std::string s : Lexer::Split::split_text(input)) {
		auto try_tks = Lexer::tokenize(s);
		if (try_tks == std::nullopt) {
			std::cerr << "Error while lexing '" + s + "'\n";
			return EXIT_FAILURE;
		}
		auto try_ast = Parser::get_ast(*try_tks);
		if (try_ast == std::nullopt) {
			std::cerr << "Error while parsing " << Lexer::print_tokens(*try_tks) << "\n";
			return EXIT_FAILURE;
		}
		auto try_result = Visitor::evaluate(*try_ast);
		if (try_result == std::nullopt) {
			std::cerr << "Error while visiting " <<
						 Parser::print_ast(*try_ast) << "\n";
			return EXIT_FAILURE;
		}
		std::cout << "result = " << Parser::print_ast(*try_result) << "\n";
	}
	return 0;
}