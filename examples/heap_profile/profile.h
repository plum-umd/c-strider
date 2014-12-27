#ifndef _PROFILE_H
#define _PROFILE_H

#include <cstrider_api.h>

struct traversal * funs_init(void);
struct traversal * funs_free(void);

/* what to do with an int/char/etc */
void profile_prim(void *in, typ type);

/* return 1 to traverse inside struct. return 0 to not traverse inside struct */
int profile_struct(void *in, typ type, void *out);

/* return a pointer to continue traversal. return NULL to not traverse this branch further */
int profile_ptr(void **in, typ type, void **out);

/* what to do with an already visted heap item */
void profile_ptr_mapped(void **in, typ type, void **out);

#endif
