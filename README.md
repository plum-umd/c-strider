c-strider
=========

C-strider is a framework for writing C heap traversals and
transformations. Writing a basic C-strider service requires implementing only four callbacks; C-strider then
generates a program-specific traversal that invokes the callbacks as each heap location is visited. Critically,
C-strider is type aware—it tracks types as it walks the heap, so every callback is supplied with the exact type
of the associated location


(TODO: post [paper] when updated)

Requires ocaml...tested with "The OCaml toplevel, version 4.00.1"

Files to look at first:
----------------------
1. **The API header**: _src/cstrider_api.h_ ( from figure 5 of paper)
2. **Template callback functions to fill out**: _src/perfaction.c_
3. **Examples of how to use**: _examples/*_
4. **Code generator:** _tools/ocaml-src/tools/cstridgen.ml_ (the user doesn't need to edit this file, but this file is the guts of cstrider's type-based traversal generation



Known limitations:
---------------------
1. Our Cil (1.3.7) code might infinite loop with Ocaml >4.02
   - Also, Cil 1.3.7 can't handle some 64 bit stuff such as _UINT64_C(0x0000000000000000)_
2. Since C-strider takes the address of struct fields to process them, **it can't handle bit fields**  (Ex: _unsigned type:4;_)
3. Does not automatically traverse static variables...you must call "visit" on them while in scope.


[paper]: http://www.cs.umd.edu/~ksaur/pubs/cstrider.pdf
