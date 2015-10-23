Lisp
====

This project is my attempt at a lisp-like language implemented in C.  As usual,
I'm more focused on implementing something new and exciting (with new
challenges).  I'm not focused on implementing a standard.  So don't expect a
complient Scheme implementation or something!

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
> (+ (+ 3 4) 3)
10
```

Current State
-------------

Here's the checklist:

- [x] Working lexer.
- [x] Working parser.
- [x] Evaluating builtin function calls.
- [x] Garbage collection.
- [ ] Lambdas, nested scopes, etc.
- [ ] Implementing a lot of builtin functions.
- [ ] Tail call optimization
- [ ] ????
- [ ] Profit!

So right now, all the functions you have are:

- `+` for addition
- `-` for subtraction or negating
- `car` for getting the first element of a list
- `cdr` for getting the rest of a list
- `length` for getting the length of a list

Unfortunately, I need to rethink the way I deal with empty lists, cause right
now it is causing segfaults :(

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
