module;
#include <type_traits>
#include <vector>
#include <string>
#include <optional>
#include <iostream>
export module Lexer;
import Description;
import Utility;
export namespace Lexer {
	// PUNCTUATION TOKENS CANNOT BE PART OF IDENTIFIERS!!!!!
	using namespace Description::Lexer;

	struct Token {
		std::string value;
		bool is_identifier;
		bool is_literal;
		bool is_punct = false;
		size_t end_index; // for the lexer only, it's where the symbol ends
		int line; // for error handling
		bool error_field = false;
		bool is_end_token = false;

	};

	template <class T> struct Check {};
	template <char C> struct Check<Punctuation<C>> {
		std::string s;
		Check(std::string _s = "") : s(_s) {}
		size_t find() {
			size_t idx = s.find(C);
			return idx;
		}
	};

	template <char... Cs>
	struct Check<Either<Punctuation<Cs>...>> {
		std::string s;
		Check(std::string _s = "") : s(_s) {}
		size_t find() {
			return (size_t) std::min(std::initializer_list<size_t>{
				Check<Punctuation<Cs>>(s).find()...
			});
		}
	};

	// an overload to make stuff a little easier
	constexpr std::vector<Token> operator+(std::vector<Token> lhs,
										   std::vector<Token> rhs) {
		lhs.insert(lhs.end(), rhs.begin(), rhs.end());
		return lhs;
	}

	Token extract_punct(std::string s, char ch, size_t si) {
		if (s[si] == ch) {
			return Token{.value = std::string{ch},
						 .is_identifier = true,
						 .is_literal = false,
						 .end_index = si+1,
						 .line = 0};
		}
		return Token{.error_field = true};
 	}

	// token extraction functions
	Token extract_word(std::string s, size_t si) {
		if (si >= s.length()) return Token{.is_end_token = true};
		std::string str = s.substr(si);
		size_t off = Check<Punctuations>(str).find();
		if (off == 0) return Token{.error_field = true};
		if (off == std::string::npos)
			return Token{.value = str,
						 .is_identifier = str[0] != '"',
						 .is_literal = str[0] == '"',
						 .end_index = off,
						 .line = 0};

		std::string tk = str.substr(0, off);
		return Token{.value = tk,
					 .is_identifier = tk[0] != '"',
					 .is_literal = tk[0] == '"',
					 .end_index = si + off,
					 .line = 0};
	}

	template <class T>
	struct Lexer { using type = Seq<>; };

	// lexing by exclusion of character classes

	template <> struct Lexer<Not<Punctuations>> {
		using type = Not<Punctuations>;
		static std::vector<Token> lex(std::string s, size_t si) {
			return {extract_word(s, si)};
		}
	};


	// for debugging
	void print_tokens(std::string label, std::vector<Token> tks) {
		std::cout << label << " { ";
		for (auto tk : tks) {
			if (tk.is_end_token) std::cout << "END ";
			else if (tk.error_field) std::cout << "ERROR ";
			else std::cout << tk.value << " ";
		}
		std::cout << "}\n";
	}

	// Lexing of repetitions ("any amount") of T
	// (T is possibly a seq or other compound stuff)
	template <class T>
	struct Lexer<Any<T>> {
		using type = Any<T>;
		static std::vector<Token> lex(std::string s, int si) {
			if (si > s.length()) return {Token{.is_end_token = true}};
			std::vector<Token> got = Lexer<T>::lex(s, si);
			std::vector<Token> ret;
			while ((got.back().error_field == false) &&
				   (got.back().is_end_token == false)) {
				ret = ret + got;
				got = Lexer<T>::lex(s, got.back().end_index);
			}
			return ret;
		}
	};

	// Lexing of strings

	template <> struct Lexer<std::string> {
		using type = std::string;
		static std::vector<Token> lex(std::string s, int si) {
			return {extract_word(s, si)};
		}
	};

	// Lexing of punctuation characters
	template <char C> struct Lexer<Punctuation<C>> {
		using type = Punctuation<C>;
		static constexpr std::vector<Token> lex(std::string s, int si) {
			return {extract_punct(s, C, si)};
		}
	};


	// Lexing of sequences

	// base case of the recursion
	template <class T> struct Lexer<Seq<T>> {
		using type = Seq<T>;
		static std::vector<Token> lex(std::string s, int si) {
			std::vector<Token> ret = Lexer<T>::lex(s, si);
			if (ret.empty()) return {Token{.is_end_token = true}};
			auto last = ret.back();
			if (last.error_field) return {Token{.error_field = true}};
			return ret;
		}
	};



	template <class T, class... Ts> struct Lexer<Seq<T, Ts...>> {
		using type = Seq<T, Ts...>;
		static std::vector<Token> lex(std::string s, int si) {
			std::vector<Token> ret = Lexer<T>::lex(s, si);
			if (ret.empty()) return {Token{.error_field = true}};
			auto last = ret.back();
			if (last.error_field == true)
				return {Token{.error_field = true}}; // error
			if (last.end_index >= s.size()) return ret;
			auto next = Lexer<Seq<Ts...>>::lex(s, last.end_index);
			if (next.back().error_field) return {Token{.error_field = true}};
			if (next.empty()) return {Token{.error_field = true}};
			return ret + next;
		}
	};

	// the main Lexer. This is used both for starting the parser
	// (a 'Form' type is just an Either<...>) and to parse an inner
	// Either<...>.

	template <> struct Lexer<Either<>> {
		using type = Either<>;
		static constexpr std::vector<Token> lex(std::string s, size_t si) {
			return {Token{.error_field = true}};
		}
	};

	template <class T, class... Ts>
	struct Lexer<Either<T, Ts...>> {
		using type = Either<T, Ts...>;
		static constexpr std::vector<Token> lex(std::string s, size_t si) {
			// try with T. if it fails, try recursively with Ts...
			auto ret = Lexer<T>::lex(s, si);
			if (ret.empty()) return Lexer<Either<Ts...>>::lex(s, si);
			if (ret[ret.size() - 1].error_field == true)
				return Lexer<Either<Ts...>>::lex(s, si);
			return ret;
		}
	};

	// a wrapper function for the user's convenience. You can also just
	// use the first line of this as a standalone invocation, it does the
	// same job.
	std::optional<std::vector<Token>> tokenize(std::string s) {
		auto lexed = Lexer<Forms>::lex(s, 0);
		
		return lexed;
	}

}