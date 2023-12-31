# Rewire

![Rewire](./rewire_pic.png "Rewire banner")  

# What is it?
Rewire is a "programming language framework". The way it works is
by editing a user-provided file (`Desription.ixx` in the
`src` directory) by following the instructions found there.  
Long story short: Rewire makes you define the syntax and (soon) the
semantics of your own programming language, with next to no
limitations.
# How complete is it at the moment?
The basic constructs for building Rewire grammars are in place:
- Not
  - This lets you specify which types of tokens you want to reject.
  - Only `Not<Punctuations>` is currently supported.
- Either
  - It lexes Exactly one (the first found) of the specified types.
  - Fully implemented but untested.
- Any
  - Accepts a single token type (possibly a compound one!) and
  - lexes it over and over until it can't no more.
  - Example (for more, see the wiki):
    - `Any<Seq<std::string, Punctuation<' '>>>`
    - The snippet above lexes (multiple instances of) any string
      (don't do this! it's just for a demonstration! it won't work!!)
      followed by a space.
    - Fully implemented and tested.
- Seq
  - Accepts a series of tokens, and lexes exactly one of them
    each, in sequence, and in the order they're provided.
    - Note: if one of the token types is an `Any`, it will
      keep parsing that `Any` until it can't anymore, and then
      it will move on. There can be multiple `Any`s in a `Seq`.
  - Fully implemented and tested.

In `Description.ixx` you'll see a bunch of stuff explicitly not
supposed to be edited by the user. Don't touch that. Later, you'll
be introduced to user-editable stuff, such as what tokens are
punctuation tokens and what form the standard construct have:

- Statement
  - This is the basic form of a statement in your language.

- Lambda
  - This will be the form of lambda functions in your language.

- Pattern
  - This will be a pattern in a pattern matching operation.
- Executable
  - This will be an external executable call, much like how a
    shell would do it.
- ProgramPipe
  - Composition of external executable, much like '|' in Bash.
- Composition
  - Function composition in your language.
- FuncDefinition
  - Function definitions in your language.

**A very important thing to note is that only the top-level
identifiers listed above must be present. You're free to
mix and match ANY character classes or identifiers listed above
to make your own complex grammars!**

As an example, this is the default Rewind syntax for
`ArgumentList`:

```cpp
using Name = Any<Not<Punctuations>>;
using ArgumentList = Seq<Any<Seq<Name,
                                 Punctuation<' '>>>
                         Name>;  
```

Here, `Name` is not hard-coded in the parser. I (user) defined it
to have precise control over what form identifiers have.
Anything that can appear in a grammar (right-hand side of the
= sign) can be user-defined with no limits.
The only thing you can't change is the set of top-level
identifiers listed above. You can't add more of them either, but
you can definitely add some specified-by-you top-level forms for
use in other top-level forms to make the code more composable and
easier to read. They're essentially macros.

# So... does it work?
Kinda. The lexer generator works with no problem with the default
configuration. But I only tested the default `Lambda` definition,
so more testing is needed. Next, when everything will work good
enough, i'll make the parser generator (would it be better to
make it a standard tree-like AST generator? Or leave the choice of
the intermediate representation to the user?...) and then the
visitor with a bunch of builtin functions, enough to make it
complete without too much predefined stuff.

# What if i want to ask questions not answered here?
See the wiki (TODO) or open an issue, or contact me on Telegram
(@SwitchAxe) and i'll be happy to answer anything.
