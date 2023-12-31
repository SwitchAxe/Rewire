module;
#include <type_traits>
#include <string>
export module Description;
export namespace Description {
	namespace Blocks {
		// dont't change Any, Either or Seq!
		template <class T> struct Any {};
		template <class... Ts> struct Seq {};
		template <class... Ts> struct Not {}; // unimplemented
		// everything after this line can be freely customized!

		// punctuation. Modify this to enable the use of
		// different tokens (you still have to provide a
		// use for them!)
		// the way to customize this is to add specializations
		// to the template, e.g. you can do
		// template <> struct Punct<'?'>
		//   : std::integral_constant<bool, true> {};
		// it's VERY important to not forget the integral constant,
		// and it must be a boolean value of true.
		// note, the lexer doesn't use THIS piece of code to determine if
		// something is or not punctuation.
		// see below for the actual discriminant.
		template <char C> struct Punctuation
			: std::integral_constant<bool, false> {};
		// The default punctuation follows. You can keep or remove it if
		// you wish.
		template <> struct Punctuation<'('>
		: std::integral_constant<bool, true> {};
		template <> struct Punctuation<')'>
		: std::integral_constant<bool, true> {};
		template <> struct Punctuation<'@'>
		: std::integral_constant<bool, true> {};
		template <> struct Punctuation<' '>
		: std::integral_constant<bool, true> {};
		template <> struct Punctuation<'|'>
		: std::integral_constant<bool, true> {};
		template <> struct Punctuation<'$'>
		: std::integral_constant<bool, true> {};
		template <> struct Punctuation<'='>
		: std::integral_constant<bool, true> {};
		template <> struct Punctuation<'+'>
		: std::integral_constant<bool, true> {};
		// Next we need to assign some use to our punctuation tokens.
		// this is done by modifying the "structure" of sentences in our
		// language. Below are some examples of all the currently supported
		// sentence types. Custom sentences are currently unavailable, so
		// you can only change the default ones, but you can change them however
		// you want to!
		// please note that these forms do not map 1:1 to tokens.
		// a statement here in its default syntax is defined as
		// "(some string)" but 'some string' can be another statement, a
		// special form, anything. The lexer will handle it accordingly, so
		// expect longer token streams than those you describe here.
		// a class template to disable automatic usage of some forms into
		// other forms is provided (the 'Not' template) but currently
		// unimplemented.
		template <class... Ts> struct Either {};
		// the 'Either' form is only used here.
		// this is the only section that doesn't actually represent
		// anything meaningful in the target language. it's for the
		// lexer, enabling it to determine what is valid punctuation
		// (and thus if to split a string there) or not.
		using Punctuations = Either<Punctuation<'+'>, Punctuation<'='>,
									Punctuation<'$'>, Punctuation<'|'>,
									Punctuation<' '>, Punctuation<'@'>,
									Punctuation<')'>, Punctuation<'('>>;

		using Statement = Seq<Punctuation<'('>,
							  Any<std::string>,
							  Punctuation<')'>>;
		using Name = Any<Not<Punctuations>>;
		using ArgumentList = Seq<Any<Seq<Name,
										 Punctuation<' '>>>,
								 Name>;
		// an identifier
		using Lambda = Seq<Punctuation<'('>,
						   ArgumentList,
						   Punctuation<')'>, 
						   Punctuation<' '>,
						   Punctuation<'@'>,
						   Punctuation<' '>,
						   std::string>;
		using Pattern = Any<Seq<Punctuation<'|'>,
								Any<Punctuation<' '>>,
								Any<std::string>,
								Punctuation<':'>,
								Any<Statement>>>;
		using Executable = Seq<Punctuation<'$'>,
							   Any<std::string>>;
		using ProgramPipe = Seq<Any<Seq<Executable,
										Punctuation<'|'>>>,
								Executable>;
		using Composition = Seq<Statement,
								Any<Seq<Punctuation<'+'>,
										Name>>>;
		using FuncDefinition = Seq<Name,
								   Any<Seq<Name,
										   Any<Punctuation<' '>>>>,
								   Punctuation<'='>,
								   Any<Statement>>;

	}
}