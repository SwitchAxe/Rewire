// Rewire.cpp : Questo file contiene la funzione 'main', in cui inizia e termina l'esecuzione del programma.
//

#include <iostream>
#include <vector>
#include <optional>
import Lexer;
int main() {
    std::optional<std::vector<Lexer::Token>> tks =
		Lexer::dispatch("(a b c d) @ foo");
	Lexer::print_tokens("Final Tokens:", *tks);
}