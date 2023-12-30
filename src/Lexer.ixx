module;
#include <type_traits>
#include <vector>
#include <string>
#include <optional>
#include <stdexcept>
#include <functional>
export module Lexer;
import Description;
import Utility;
export namespace Lexer {
	// PUNCTUATION TOKENS CANNOT BE PART OF IDENTIFIERS!!!!!

	template <char C> concept is_punct =
		std::is_same_v<Description::Blocks::Punctuation<C>,
					   std::integral_constant<bool, true>>;
	using namespace Description::Blocks;
	
	template <class T> struct Parameter { using type = T; };

	std::vector<std::string> tokens(std::string stream) {
		return {};
	}

	struct Token {
		std::string value;
		bool is_identifier;
		bool is_literal;
		int end_index; // for the lexer only, it's where the symbol ends
		int line; // for error handling
	};

	template <class... Ts> struct Lex {};

	template <class T> struct Lex<Any<T>> {
		static constexpr std::vector<Token> tokens(std::string s) {
			return Lex<T>::tokens(s);
		}
	};

	// token extraction functions
	constexpr Token extract_word(std::string s) {
		int idx = s.find(' ', 0);
		std::string str = s.substr(0, idx);
		return Token{.value = str,
					 .is_identifier = str[0] != '\"',
					 .is_literal = str[0] == '\"',
					 .end_index = idx,
					 .line = 0};
	}

	constexpr Token extract_punct(std::string s, char ch) {
		if (s[0] == ch) {
			return Token{.value = std::string{ch},
						 .is_identifier = true,
						 .is_literal = false,
						 .end_index = 0,
						 .line = 0};
		}
 	}

	template <class T>
	struct Lexer {};

	template <class T>
	struct Lexer<Any<T>> {
		static constexpr std::vector<Token> lex(std::string s) {
			auto got = Lexer<T>::lex(s);
			std::vector<Token> ret;
			while (got.length() > 0) {
				ret.insert(ret.end(), got.begin(), got.end());
				got = Lexer<T>::lex(s);
			}
			return ret;
		}
	};

	template <> struct Lexer<std::string> {
		static constexpr std::vector<Token> lex(std::string s) {
			return std::vector<Token>{extract_word(s)};
		}
	};

	template <char C> struct Lexer<Punctuation<C>> {
		static constexpr std::vector<Token> lex(std::string s) {

		}
	};

	template <class... Ts> struct Lexer<Seq<Ts...>> {
		static constexpr std::vector<Token> lex(std::string s) {

		}
	};

	std::optional<std::vector<Token>> dispatch(std::string s) {
		auto lambda_lex = Lexer<std::decay_t<Lambda>>::lex(s);
		
		return std::nullopt;
	}

	std::vector<Token> lex(std::string s) {
		int idx = 0;
		std::vector<Token> ret;
		auto cpy = s;
		while (idx < s.length()) {}
	}

}