// Rewire.cpp : Questo file contiene la funzione 'main', in cui inizia e termina l'esecuzione del programma.
//

#include <iostream>
#include <vector>
#include <optional>
import Lexer;
int main() {
    std::optional<std::vector<Lexer::Token>> tks =
		Lexer::dispatch("(a b c d) @ foo");
	std::cout << "tokens: \n";
	for (Lexer::Token tk : *tks) {
		std::cout << tk.value << "\n";
	}
}