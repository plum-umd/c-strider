c-strider
=========

(TODO: post [paper] when updated)

Requires ocaml...tested with "The OCaml toplevel, version 4.00.1"

API: src/cstrider_api.h ( from figure 5 of paper)
Template: src/perfaction.c
Code generator: tools/ocaml-src/tools/cstridgen.ml



Known limitations:
---------------------
1. Our Cil (1.3.7) code might infinite loop with Ocaml >4.02
   - Also, Cil 1.3.7 can't handle some 64 bit stuff such as _UINT64_C(0x0000000000000000)_
2. Since C-strider takes the address of struct fields to process them, **it can't handle bit fields**  (Ex: _unsigned type:4;_)
3. Does not automatically traverse static variables...you must call "visit" on them while in scope.


[paper]: http://www.cs.umd.edu/~ksaur/pubs/cstrider.pdf
