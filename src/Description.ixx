module;
#include <type_traits>
#include <string>
export module Description;
export namespace Description {
	namespace Blocks {
		// dont't change Any, Either or Seq!
		template <class T> struct Any {};
		template <class T, class U> struct Either {}; // unimplemented
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
		// it's VERY important to not forget the integral constant, and it must
		// be a boolean value of true.
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
		using Statement = Seq<Punctuation<'('>,
							  Any<std::string>,
							  Punctuation<')'>>;
		using ArgumentList = Any<Seq<Any<std::string>,
									 Any<Punctuation<' '>>>>;
		using Lambda = Seq<Punctuation<'('>,
						   Any<ArgumentList>,
						   Punctuation<')'>, 
						   Any<Punctuation<' '>>,
						   Punctuation<'@'>,
						   Any<Punctuation<' '>>,
						   Any<Statement>>;
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
		using Name = Any<Not<Punctuation<' '>, Punctuation<'='>>>;
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