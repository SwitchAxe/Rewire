module;
#include <type_traits>
#include <string>
export module Description;
import Types;
export namespace Description {
	namespace Lexer {
		// dont't change Any, Either, Seq or Not!
		template <class T> struct Any {};
		template <char C> struct Punctuation {};
		template <class... Ts> struct Either {};
		template <class... Ts> struct Seq {};
		template <class... Ts> struct Not {}; // unimplemented
		// everything after this line can be freely customized!

		// We need to define some punctuation and then
		// assign some use to our punctuation tokens.
		// this is done by modifying the "structure" of sentences in our
		// language. Below are some examples of some possible sentence types.
		// please note that these forms do not map 1:1 to tokens.
		// a statement here in its default syntax is defined as
		// "(some string)" but 'some string' can be another statement, a
		// special form, anything. The lexer will handle it accordingly, so
		// expect longer token streams than those you describe here.
		// a class template to disable automatic usage of some forms into
		// other forms is provided (the 'Not' template) but currently
		// unimplemented.

		// this is the only section that doesn't actually represent
		// anything meaningful in the target language. it's for the
		// lexer, enabling it to determine what is valid punctuation
		// (and thus if to split a string there) or not.
		using Punctuations = Either<Punctuation<'+'>, Punctuation<'='>,
									Punctuation<'$'>, Punctuation<'|'>,
									Punctuation<' '>, Punctuation<'@'>,
									Punctuation<')'>, Punctuation<'('>,
									Punctuation<':'>, Punctuation<'\\'>,
									Punctuation<'\n'>>;

		using LineEndToken = Punctuation<'\n'>;
		using LineContinuation = Seq<Punctuation<'\\'>,
									 Punctuation<'\n'>>;

		// The only builtin section that's also a template.
		template <class... Ts>
		using EnableLineContinuation = Any<Either<Ts..., LineContinuation>>;

		// an identifier
		using Name = Any<Not<Punctuations>>;
		using ArgumentList = Seq<Any<Seq<Name,
										 Punctuation<' '>>>,
								 Name>;

		using Statement = Seq<Punctuation<'('>,
							  ArgumentList, // this doubles as a function call!
							  Punctuation<')'>>;

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
		using Executable = Seq<Punctuation<'$'>, Name>;
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

		// edit this to enable the use of all the forms you wish to lex strings
		// against.
		using Forms =  Either<FuncDefinition, Name, Statement,
							  ArgumentList, Composition, ProgramPipe,
							  Executable, Pattern, Lambda>;

	}

	namespace Parser {
		// first some aux structs (don't remove or change these!!)
		
		// please use these according to the instructions found later.
		template <class Ts> struct Repeat {}; // Any<T> but for the parser
		template <class... Ts> struct List {};
		// A List, but wraps (nests) its arguments.
		// totally equivalent to List<List<Ts...>> but less ugly.
		// List<a, b, List<c, d>> = [a, b, c, d]
		// List<a, b, Wrap<c, d>> = [a, b, [c, d]]
		template <class... Ts> struct Wrap {};
		struct Identifier {};
		struct Literal {};
		// ignore whatever token is found at that position in the
		// tree (see the next structs), used primarily for punctuation
		// in the default grammar, but it can be used for more complex
		// stuff, possibly.
		// The parser basically skips to the next Token if it finds
		// this type.
		template <class T> struct Ignore {};
		struct None {}; // used as a default for the next struct
		// The Rewire parser generator is recursive in nature, this means that
		// you'll need to provide a recursive structure to hold the data.
		// This is conveniently provided to you by 'Types.ixx', which contains
		// several Symbol structs for various data types. Soon you'll also be
		// able to provide your own data types.
		// The configurable part of the parser relates to how the tree structure
		// of your language must be built based on the token types.
		// you must provide exactly one template specialization of
		template <class T> struct Describe { using what = None; };
		// for each form that appears in 'Forms' (See the Lexer namespace of this
		// file for a detailed explanation).
		// see below for the structure of this type.
		using namespace Lexer;
		using namespace Types;

		// You can also define auxillary types here!
		// those don't need to have a Describe<T> instance
		// unless they must have a specific form. In this case
		// it's just a macro so we can live without specifying
		// its structure further.
		// Be aware that if you need to also lex this thing
		// correctly (because, for instance, it's a complex form)
		// you'll need to define it in the Lexer namespace above and
		// possibly (depending on your goal) add it to the Forms
		// directive!
		using Argument = Either<Statement, Executable, Name,
								ProgramPipe, Composition, Pattern>;


		template <> struct Describe<Statement> {
			// This is where you tell the parser what
			// your form describes. In this case, it describes a list
			// recursively composed of statements or other expressions.
			using what = List<Ignore<Punctuation<'('>>,
							  ArgumentList,
							  Name,
							  Ignore<Punctuation<')'>>>;
		};


		template <> struct Describe<ArgumentList> {
			// the same logic applies here. Clearly an argument list
			// can't contain another argument list.
			using what = Repeat<List<Argument,
									   Ignore<Punctuation<' '>>>>;
		};

		template <> struct Describe<Name> {
			// Names are just Identifiers
			using what = Not<Punctuations>;
		};

		template <> struct Describe<Executable> {
			// an executable can have as arguments
			// an argument list, a statement, ...
			// that is, any argument a statement accepts:
			using what = List<Punctuation<'$'>, Name, ArgumentList, Name>;
		};

		template <> struct Describe<FuncDefinition> {
			using what = List<Name, List<Name>, Punctuation<'='>,
							  List<Repeat<Either<Statement, Executable,
											  ProgramPipe, Composition>>>>;
		};

		template <> struct Describe<Composition> {
			using what = List<Name,
							  Repeat<List<Punctuation<'+'>, Name>>,
							  Repeat<ArgumentList>>;
		};

		template <> struct Describe<ProgramPipe> {
			using what = List<Executable,
							  Repeat<ArgumentList>,
							  Repeat<List<Punctuation<'|'>,
									   Executable,
									   Repeat<ArgumentList>>>,
							  Repeat<ArgumentList>>;
		};

		template <> struct Describe<Pattern> {
			using what = List<Punctuation<'|'>,
							  Name,
							  Punctuation<':'>,
							  Repeat<Statement>>;
		};
	}
}