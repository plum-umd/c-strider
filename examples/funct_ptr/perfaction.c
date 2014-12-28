
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <cstrider_api.h>


/* action for a pointer discovery.
 * return next place to visit. */
int perfaction_ptr(void **in, typ type, void **out)
{
   return 1;
}

/* action for a pointer re-discovery.
 * Last parameter (ret_val)  is "returned".*/
void perfaction_ptr_mapped(void **in, typ type, void **out)
{

}

/* action for struct discovery */
int perfaction_struct(void *in, typ type, void *out)
{
   return 1;
}

void perfaction_prim(void *in, typ type, void *out)
{

}

struct traversal funs  =
{
   .perfaction_prim =&perfaction_prim,
   .perfaction_struct = &perfaction_struct,
   .perfaction_ptr = &perfaction_ptr,
   .perfaction_ptr_mapped = &perfaction_ptr_mapped
};
