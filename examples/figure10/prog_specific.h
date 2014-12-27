#ifndef PROG_SPECIFIC_H
#define PROG_SPECIFIC_H
struct traversal * union_funs_init(void);
int union_ptr(void **in, typ type, void **out);
void union_ptr_mapped(void **in, typ type, void **out);
int union_struct(void *in, typ type, void *out);
void union_prim(void *in, typ type, void *out);


extern int TYPE_STRUCT_tagged_union_i, TYPE_INT_i, TYPE_CHAR_i;

#endif
