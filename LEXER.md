Lexer Documentation
===================

My lexer ([`src/lex.c`](src/lex.c)) is a very generalized lexer.  Ideally, it
could do the tokenizing steps for most programming languages, especially the
C-like ones.  Instead of writing code tailored to the task of lisp's tokens, I
created this lexer and defined lisp's tokens as regular expressions.

The lexer uses my libstephen regular expression engine.  Essentially, it has a
list of tokens and corresponding regular expressions.  It reads input character
by character, until every NDFSM has rejected the input.  Then, it looks back at
the last accepting regex and returns that token, along with the string
corresponding to it.

So for lisp, all I need to do is create a lexer instance with the tokens I
define, and I have a ready-made lexer!  (By the way, that function is
`lisp_create_lexer()` in [`src/parse.c`](src/parse.c)).
