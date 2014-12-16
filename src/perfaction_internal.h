#ifndef _PERFACTION_OPT
#define _PERFACTION_OPT

typedef long typ;

/**** the user-implemented API ****/

//TODO first two are deprecated
/* called at the beginning. initialize stuff, open files, etc */
void perfaction_init(void);

/* called at the very end of traversal. free stuff, close files, etc */
void perfaction_free(void);

/* what to do with an int/char/etc */
void perfaction_prim(void *in, typ type, void *out);

/* return 1 to traverse inside struct. return 0 to not traverse inside struct */
int perfaction_struct(void *in, typ type, void *out);

/* return a pointer to continue traversal. return NULL to not traverse this branch further */
int perfaction_ptr(void **in, typ type, void **out);

/* what to do with an already visted heap item */
void perfaction_ptr_mapped(void **in, typ type, void **out);



#endif
