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
	using namespace Description::Blocks;

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
			std::cout << "in check S = <" << s << "> and C = " << C << "\n";
			size_t idx = s.find(C);
			std::cout << "in check find idx = " << idx << "\n";
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
		std::cout << "searching punct for '" << ch << "'\n";
		if (s[si] == ch) {
			return Token{.value = std::string{ch},
						 .is_identifier = true,
						 .is_literal = false,
						 .end_index = si+1,
						 .line = 0};
		}
		std::cout << "failed to find punct!\n";
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
		std::cout << "SI + OFF = " << si + off << "\n";
		return Token{.value = tk,
					 .is_identifier = tk[0] != '"',
					 .is_literal = tk[0] == '"',
					 .end_index = si + off,
					 .line = 0};
	}

	template <class T>
	struct Lexer {};

	// lexing by exclusion of character classes

	template <> struct Lexer<Not<Punctuations>> {
		static std::vector<Token> lex(std::string s, size_t si) {
			std::cout << "IN NOT PUNCT SI = " << si << "\n";
			return {extract_word(s, si)};
		}
	};

	// lexing of "several options", i.e. lexing of
	// at least one/only one alternatives in a set of
	// possibilities.

	// base case
	template <> struct Lexer<Either<>> {
		static constexpr std::vector<Token> lex(std::string s, size_t si) {
			return {Token{.error_field = true}};
		}
	};

	template <class T, class... Ts>
	struct Lexer<Either<T, Ts...>> {
		static constexpr std::vector<Token> lex(std::string s, size_t si) {
			// try with T. if it fails, try recursively with Ts...
			auto ret = Lexer<T>::lex(s, si);
			print_tokens("Either<Ts...> [start]", ret);
			if (ret.empty()) return Lexer<Either<Ts...>>::lex(s, si);
			if (ret[ret.size() - 1].error_field == true)
				return Lexer<Either<Ts...>>::lex(s, si);
			return ret;
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
		static std::vector<Token> lex(std::string s, int si) {
			if (si > s.length()) return {Token{.is_end_token = true}};
			std::vector<Token> got = Lexer<T>::lex(s, si);
			print_tokens("got", got);
			std::vector<Token> ret;
			while ((got.back().error_field == false) &&
				   (got.back().is_end_token == false)) {
				ret = ret + got;
				got = Lexer<T>::lex(s, got.back().end_index);
			}
			print_tokens("Any<T> [END]", ret);
			return ret;
		}
	};

	// Lexing of strings

	template <> struct Lexer<std::string> {
		static std::vector<Token> lex(std::string s, int si) {
			return {extract_word(s, si)};
		}
	};

	// Lexing of punctuation characters
	template <char C> struct Lexer<Punctuation<C>> {
		static constexpr std::vector<Token> lex(std::string s, int si) {
			return {extract_punct(s, C, si)};
		}
	};


	// Lexing of sequences

	// base case of the recursion
	template <class T> struct Lexer<Seq<T>> {
		static std::vector<Token> lex(std::string s, int si) {
			std::vector<Token> ret = Lexer<T>::lex(s, si);
			if (ret.empty()) return {Token{.is_end_token = true}};
			auto last = ret.back();
			if (last.error_field) return {Token{.error_field = true}};
			return ret;
		}
	};



	template <class T, class... Ts> struct Lexer<Seq<T, Ts...>> {
		static std::vector<Token> lex(std::string s, int si) {
			std::vector<Token> ret = Lexer<T>::lex(s, si);
			print_tokens("Seq<T>", ret);
			if (ret.empty()) return {Token{.is_end_token = true}};
			auto last = ret.back();
			if (last.error_field == true)
				return {Token{.error_field = true}}; // error
			if (last.end_index >= s.size()) return ret;
			auto next = Lexer<Seq<Ts...>>::lex(s, last.end_index);
			if (next.back().error_field) return {Token{.error_field = true}};
			if (next.empty()) return {Token{.error_field = true}};
			print_tokens("Seq<T> [END]", ret + next);
			return ret + next;
		}
	};

	std::optional<std::vector<Token>> dispatch(std::string s) {
		auto lambda_lex = Lexer<Lambda>::lex(s, 0);
		
		return lambda_lex;
	}

}