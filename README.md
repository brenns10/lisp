Lisp
====

This project is my attempt at a lisp-like language implemented in C.  As is
normal for my projects, my focus is on implementing something I've never done
before and learning how it's done.  I'm not focused on implementing any
Lisp/Scheme standard, although I'm relying heavily on my knowledge of Scheme.

All that being said, the interpreter is already in a surprisingly functional
(haha) state.  I have implemented lambda functions and function calls, basic
integer arithmetic, simple list operations, if statements, and I even have a
basic garbage collection system (via reference counting) set up.  It's pretty
easy to try out (see below).

Run It!
-------

If you have a Unix-y development environment, it should be easy to get this up
and running:

```bash
$ git clone git@github.com:brenns10/lisp.git
$ cd lisp
$ git submodule init
$ git submodule update
$ make
$ bin/release/main
> (define increment (lambda (x) (+ x 1)))
(lambda
 (
  x
 )
 (+

  x
  1
 )
)
> (increment 5)
6
> (if 0 (exit) 5)
5
> (exit)
0
```

Current State
-------------

Here's the checklist:

- [x] Working lexer.
- [x] Working parser.
- [x] Evaluating builtin function calls.
- [x] Garbage collection.
- [x] Lambdas, nested scopes, etc.
- [ ] Closures for lambdas
- [ ] Implementing more builtins:
    - [x] `cons`, `car`, `cdr`, `length` (list stuff)
    - [x] `lambda`, `define` (basic function stuff)
    - [x] `if`
    - [ ] `cond`
    - [ ] `begin`
    - [ ] `let` family of functions
    - [ ] `map`, `fold`, `call`, `apply`, etc: the higher order functions
    - [ ] comparisons - `=`, `eq?`, `equal?`, `string=`, ...
- [ ] A builtin boolean type (currently false is integer 0, and true is anything
  else)
- [ ] Error handling that doesn't involve `exit(EXIT_FAILURE)`.
- [ ] Tail call optimization
- [ ] ????
- [ ] Profit!

What is available in the language right now:

- `+` for addition
- `-` for subtraction or negating
- `car` for getting the first element of a list
- `cdr` for getting the rest of a list
- `cons` for putting an element onto the front of a list
- `length` for getting the length of a list
- `if` for if statements (branch not taken is not evaluated!)
- `lambda` for creating a function (**closures aren't yet supported**)
- `define` for binding a name to your current scope
- `=`, `<`, `>`, `<=`, `>=`, for comparing integers

The Code
--------

I think the code is pretty darn interesting.  I have written some documents to
help people understand how my code works.  There is one describing the
[lexer](LEXER.md), one describing the [garbage collection system](GARBAGE.md),
and one [that takes you through an example lisp input](EXAMPLE.md), all the way
through the interpreting process!  Check them out if you're interested in
learning how my code works.  You may also be very interested in looking at my
[libstephen documentation](http://stephen-brennan.com/libstephen) if you see a
lot of function calls you don't see definitions for.

Contributing
------------

If you wanna PR, I'll consider 'em!

License
-------

Copyright (c) 2015, Stephen Brennan  
All rights reserved.  
Released under the Revised BSD License, which you may find in
[LICENSE.txt](LICENSE.txt).
