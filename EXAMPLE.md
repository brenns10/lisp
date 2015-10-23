Example
=======

In this document, I'll trace through the execution of the interpreter for a
small code snippet, and hopefully this will illustrate a lot of how the
interpreter works.

Let's assume that the input is `(length '(1 2 3))`.  The value of this
expression is 3.  How does the interpreter get that value?  Well, let's start
with the main function.

`lisp_lex_file()`
-----------------

Currently, `main()` simply calls `lisp_interact()` in [`eval.c`](src/eval.c).
`lisp_interact()` will start lexing from `stdin` by calling `lisp_lex_file()`
(in [`parse.c`](src/parse.c)).  All this function actually does is return a
`smb_iter` struct.  This is something I created to represent the abstraction of
an iterator.  It contains a couple data fields, and function pointers for
`next()` and `has_next()`.  All of the parsing functions will ask for tokens
from this iterator by calling its `next()` function.  So let's look at this
`next()` function pointer (we can see that it is `lisp_lex_file_next()`, defined
in [`parse.c`](src/parse.c) as well.

This function asks my `smb_lex` object for the next token out of the file
(which, recall, is stdin).  I won't go into detail on the calls for `smb_lex`.
All you need to know is that it runs several regular expressions on the input
and returns the token that has the largest match.  If it's whitespace, my lexer
actually just ignores it.  Otherwise, if it's a token that my interpreter will
need the text for (like an identifier or a number), it keeps that stuff and
returns the token.  If it doesn't need the text, it frees the text and then
returns the token.

The `has_next()` function of the iterator simply checks if we're at the end of
the file yet.

So, what we've got is an iterator that will return tokens every time you call
the `next()` function.  Awesome.  For our particular example, this iterator will
return:

- `OPEN_PAREN`
- `IDENTIFIER`, `length`
- `OPEN_LIST`
- `INTEGER`, `1`
- `INTEGER`, `2`
- `INTEGER`, `3`
- `CLOSE_PAREN`
- `CLOSE_PAREN`

Now, we look at the next thing done by `lisp_interact()`.

`lisp_create_globals()`
-----------------------

This function (defined in [`functions.c`](src/functions.c)) takes all the
built-in functions I have defined so far, stuffs them into a hash table, and
returns them.  These built-in functions are created as `lisp_values`, so the
hash table is what "owns" the references to them.  Once the scope is deleted,
those builtins should be decref'd to 0 and deleted as well.

The code we're about to interpret uses only one built-in: `length`.  This is
defined as `lisp_length()` in the same file.  It is a very simple function that
actually just ends up calling a helper to count the number of nodes there are in
a linked list.  It then returns that number in a new `lisp_int` object.

The Main Loop
-------------

Now, `lisp_interact()` enters its main loop.  It has the scope necessary to
execute the code, and it has the token stream ready to start tokenizing.  It
calls `lisp_parse()` (in [`parse.c`](src/parse.c))next, with a pointer to the
iterator.

### `lisp_parse()`

What this will do is take each token and convert it into a `lisp_value`
representing the code.  For code like `5`, `'a`, or `'(a b c)`, this just
returns the corresponding integer, atom, or list.  For code like `(+ 1 2)`, it
recognizes a function call and returns a `lisp_funccall` object, which has
references to the function as well as its arguments (which are `lisp_value` and
`lisp_list` respectively).

The parse tree for our code would be as follows:

- `lisp_funcall`:
    - function: `lisp_identifier`, value=`length`
    - arguments: `lisp_list`
        - value: `lisp_list`
            - value: `lisp_int`, value=`1`
            - next: `lisp_list`
                - value: `lisp_int`, value=`2`
                - next: `lisp_list`
                    - value: `lisp_int`, value=`3`
                    - next: `null`
        - next: `null`

`lisp_parse()` is a recursive function that will return once it has parsed a
complete expression.  Furthermore, every object created by `lisp_parse()` has
only one reference.  If the object isn't at the top level, then it is owned by
its parent object in the parse tree.  At the end of the day, the root of the
parse tree has its reference returned to the caller.  If the caller decref's the
value returned by `lisp_parse()`, this will kick off a chain reaction of
decref-ing that wil free the entire parse tree.  Now that's pretty cool!

### `lisp_evaluate()`

The next part of the code is to call `lisp_evaluate()` (in
[`eval.c`](src/eval.c)).  This will return the value of any lisp expression.
Most lisp data types remain the same when evaluated, except for function calls
and identifiers.  For identifiers, it simply looks them up in the scope when
they're evaluated.  When function calls are evaluated, `lisp_evaluate()`
evaluates the function.  In this case, the function is a `lisp_identifier`, and
so this triggers a hash table lookup in the scope.  A NEW reference to the
length function is returned.  Then, we evaluate the function arguments.  Since
these are all already evaluated, they simply return new references to the same
things.  Finally, `lisp_evaluate()` runs the builtin function on its arguments.
It decrefs the function and the arguments (since it's done with them), and
returns the function's return value.  Since the length of that list was 3,
`lisp_evaluate()` returns a `lisp_int` containing the value 3.

### `tp_print(res)`

Wow, so we've pretty much done all of the interpreting.  All the interpreter has
to do now is print out the value that got returned, and exit.  In order to print
an object, the interpreter needs to know what type it is.  Thankfully, my type
objects contain a function pointer that knows how to print out that type of
object.  So, we look up the type object's `tp_print` function and call it on the
result.  Finally, we decref the code, since we're done with it, and we also
decref the result, since we're done with it too.  Since the tokens were freed by
`lisp_parse()`, this ends up freeing all of the memory allocated by the
interpreter, except for the scope and the lexer, which we do just before
exiting.


TADA!
