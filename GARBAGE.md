Garbage Collection
==================

In my opinion, garbage collection is the most magical part of this program so
far.  My garbage collection is implemented with reference counting (no cycle
detection -- I'll cross that bridge when I get to it).  I borrowed quite heavily
from CPython's reference counting implementation (at least, what I know of it
from working on a C extension).  Here's how everything works:

The lisp_type
-------------

In CPython, there are these things called "type objects".  These are statically
allocated structs that contain information and methods that have to do with
certain types of objects.  The most important functions in these type objects
are functions that will allocate and deallocate a new object of that type.  When
you're doing garbage collection, you really need to know how to do allocation
and deallocation of any type of object.  So, I copied this idea wholesale from
CPython.

Every type in the language has a type object.  Some of the interesting ones now
are:

- `tp_int` - for integers
- `tp_atom` - for atmoms
- `tp_builtin` - for built in functions
- `tp_funccall` - for code representing function calls

My type objects define string names as well as functions that perform
allocation, deallocation, and printing.  You can see this stuff more in the
definition of `lisp_type` in [`src/lisp.h`](src/lisp.h).

The lisp_value
--------------

All language objects are "subclasses" of `lisp_value`.  What this means is that
they contain a `lisp_value` struct at the top of their definition.  `lisp_value`
contains two important fields:

- `type` - a pointer to the object's type
- `refcount` - number of references to this object currently "out there"

Whenever you call an `alloc()` function for a type, it sets the `type` to
itself, and the `refcount` to 1 automatically.  The functions `lisp_incref()`
and `lisp_decref()` update reference counts, and `lisp_decref()` will deallocate
if the refcount hits 0.

Owning references
-----------------

In order for this to work correctly, you need to have a pretty decent
understanding of what code "owns" what references (i.e. pointers to objects).
Most of the time, if a function is called with a reference, it is just borrowing
the reference, and shouldn't decref it.  Also most of the time, if a function
returns a reference, it probably returns a "new" reference that the caller will
own, and thus have to decref when it's done with it.

Python has some
[excellent documentation](https://docs.python.org/3/extending/extending.html#reference-counts)
on how this works.  If you're interested in reference ownership semantics, or
want a clearer explanation of reference counting, you'll probably find that
Python's documentation will end up helping you understand what I've done.
