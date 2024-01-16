module;
#include <type_traits>
#include <string>
export module Description;
import Types;
import Strings;
export namespace Description {
	namespace Lexer {
		// dont't change Any, Either, Seq or Not!
		template <class T> struct Any {};
		template <char C> struct Punctuation {};
		template <Strings::String s> struct Keyword {};
		template <class... Ts> struct Either {};
		template <class... Ts> struct Seq {};

		// You can freely use some default token types:
		struct Identifier {};
		struct Number {};
		struct Boolean {}; // "true" and "false"
		struct String {}; // string literals

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
									Punctuation<':'>, Punctuation<'#'>,
									Punctuation<'<'>, Punctuation<'>'>>;

		// Now we need to add some language keywords. These are reserved for
		// specific things in the language. These can be anything, not necessarily
		// words composed of alphabet characters.
		// these will be matched exactly by the lexer, and then
		// the parser.
		using Keywords = Either<Keyword<"->">, Keyword<"let">>;

		// modify these according to which newline token you want and
		// which newline with continuation token you want.
		// the newline with continuation token is basically
		// a newline which does NOT end the current expression.
		// print(a, b <newline>, c) is most likely invalid, since
		// it produces two expressions: "print(a, b" and ",c", which
		// then get lexed and parsed individually.
		// with continuation, the newline token is ignored, so
		// the above statement gets lexed and parsed whole,
		// but the user can still write it on different lines for
		// readability.
		using LineEndToken = Punctuation<'\n'>;
		using LineContinuation = Punctuation<'\\'>;

		using ArgumentList = Seq<Any<Seq<Either<Identifier, Number, Boolean, String>,
										 Punctuation<' '>>>,
								 Either<Identifier, Number, Boolean, String>>;

		using Statement = Seq<Punctuation<'('>,
							  ArgumentList, // this doubles as a function call!
							  Punctuation<')'>>;

		using Lambda = Seq<Punctuation<'('>,
						   ArgumentList,
						   Punctuation<')'>, 
						   Punctuation<' '>,
						   Punctuation<'@'>,
						   Punctuation<' '>,
						   Statement>;

		using Clause = Either<Seq<Punctuation<'<'>, Number>,
							  Seq<Identifier, Punctuation<'<'>, Number>,
							  Seq<Punctuation<'>'>, Number>,
							  Seq<Identifier, Punctuation<'>'>, Number>>;

		using Pattern = Any<Seq<Punctuation<'|'>,
								Any<Punctuation<' '>>,
								Clause,
								Punctuation<':'>,
								Any<Statement>>>;

		using Composition = Seq<Statement,
								Any<Seq<Any<Punctuation<' '>>,
										Punctuation<'|'>,
										Any<Punctuation<' '>>,
										Statement>>>;
		using FuncDefinition = Seq<Identifier,
								   Any<Seq<Identifier,
										   Any<Punctuation<' '>>>>,
								   Punctuation<'='>,
								   Any<Statement>>;

		using VariableDefinition = Seq<Identifier,
									   Any<Punctuation<' '>>,
									   Punctuation<'='>,
									   Any<Punctuation<' '>>,
									   Either<Identifier,
											  Statement,
											  Composition,
											  Pattern>>;

		// edit this to enable the use of all the forms you wish to lex strings
		// against.
		using Forms =  Either<FuncDefinition, Identifier, Statement,
							  ArgumentList, Composition, Pattern, Lambda,
							  VariableDefinition>;

	}

	namespace Parser {
		// first some aux structs (don't remove or change these!!)
		
		// please use these according to the instructions found later.
		template <class Ts> struct Repeat {}; // Any<T> but for the parser
		// Optional either parses T successfully or silently moves on. e.g.
		// List<Optional<a>, b, c> either parses [a, b, c] or [b, c].
		// Repeat<Optional<T>> is illegal.
		template <class T> struct Optional {};
		template <class... Ts> struct List {};
		// Wraps its argument into a List.
		// List<a, b, List<c, d>> = [a, b, c, d]
		// List<a, b, Wrap<c>> = [a, b, [c]] and thus
		// List<a, b, Wrap<List<c, d>>> = [a, b, [c, d]]
		template <class T> struct Wrap {};
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

		using Literal = Either<String, Number, Boolean>;

		using Argument = Either<Literal, Identifier, Composition, Pattern>;


		template <> struct Describe<Statement> {
			// This is where you tell the parser what
			// your form describes. In this case, it describes a list
			// recursively composed of statements or other expressions.
			using what = List<Ignore<Punctuation<'('>>,
							  ArgumentList,
							  Ignore<Punctuation<')'>>>;
		};

		template <> struct Describe<VariableDefinition> {
			using what = List<Identifier,
							  Optional<Repeat<Punctuation<' '>>>,
							  Punctuation<'='>,
							  Optional<Repeat<Punctuation<' '>>>,
							  Argument>;
		};

		template <> struct Describe<ArgumentList> {
			// the same logic applies here. Clearly an argument list
			// can't contain another argument list.
			using what = List<Repeat<List<Argument,
										  Ignore<Repeat<Punctuation<' '>>>>>,
							  Optional<Argument>>;
		};


		template <> struct Describe<FuncDefinition> {
			using what = List<Identifier,
							  Optional<Repeat<Identifier>>,
							  Punctuation<'='>,
							  Repeat<Argument>>;
		};

		template <> struct Describe<Composition> {
			using what = List<Ignore<Punctuation<'('>>,
							  ArgumentList,
							  Ignore<Punctuation<')'>>,
							  Optional<Ignore<Repeat<Punctuation<' '>>>>,
							  Ignore<Punctuation<'|'>>,
							  Optional<Ignore<Repeat<Punctuation<' '>>>>,
							  Wrap<Either<Composition, Statement>>>;
		};

		template <> struct Describe<Pattern> {
			using what = List<Punctuation<'#'>,
							  Identifier,
							  Punctuation<':'>,
							  Repeat<Statement>>;
		};
	}

	namespace Eval {
		// This is the last namespace. Here you're gonna tell the visitor which
		// token types have special meaning, and which meaning they have.
		// these are useful so that a function definition and
		// a program call are not evaluated in the same way.
		using namespace Parser;

		struct None {};
		// this is the struct we're going to assign stuff to.
		// (for custom meanings, TODO)
		template <class T> struct Meaning { using what = None; };

		// Any means Any symbol type is accepted. (stops at punctuation tokens)
		struct Any {};

		// modify the structs below to have their respective
		// meanings mapped 1:1 to a token sequence template.
		// remember: you're free to use any class from the parser.
		// in particular, you can use Boolean, Number, etc.
		// The way these works is that for each type, the visitor
		// checks the AST produced by your parser, and chooses
		// **the first match** as the meaning to visit.
		// in each of these templates, you can freely choose which
		// types are used, in which order, and amount.
		// however, the structure must be followed.
		// some examples will clarify this.

		// Each T in Ts... is a token type.
		// these tokens get scanned sequentially in the AST.
		// if a List is met, a List is expected in the AST.
		// each of these tokens can contain a Let<> (no type)
		// which gets parsed following this rule:
		// the first Let<T> found will be a function name.
		// the subsequent n Let<> identifiers will be function parameters.
		// after the last Let<> is found, the special
		// LetEnd token is expected. This means that from that
		// point onward, everything else is the body of the function.
		// e.g.:
		//Let<Let<>,
		//	  Punctuation<'(>,
		//	  Repeat<List<Let<>, Punctuation<' '>>>,
		//    Punctuation<')>
		//    LetEnd, Statement>
		// defines a function like
		// foo(a, b, c) ...
		// where foo is an identifier, and
		// a, b and c are parameters to the function.
		template <class... Ts> struct Let {};
		struct LetEnd;
		// In order the types are:
		// a type to use for the identifier;
		// a type to use for the type of the variable;
		// a type (possibly `Any`) to use for the value.
		// if the second type is not present, any value type
		// can be available to the variable.
		template <class... Ts> struct Var {};

		// statements. don't use this if your language doesn't have
		// statements. The structure of State is an token type
		// fitting for identifiers, and some form of list as arguments.
		template <class... Ts> struct State {};

		// everything after this comment is just a default.
		// change or remove it appropriately if you wish.


		template <> struct Meaning<Statement> {
			using what = State<Identifier, Repeat<Any>>;
		};

		// choose which Meaning specializations to use.
		using ToEval = Either<Identifier, Literal, Statement>;


	}
}