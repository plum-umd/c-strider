
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "perfaction_internal.h"


/*TODO remove these */
void perfaction_init()
{
   profile_init();
}

void perfaction_free()
{
   profile_free();
}

int perfaction_ptr(void **in, typ type, void **out)
{
   return profile_ptr(in ,type, out);
}

void perfaction_ptr_mapped(void **in, typ type, void **out)
{
   profile_ptr_mapped(in, type, out);
}

int perfaction_struct(void *in, typ type, void *out)
{
   return profile_struct(in, type, out);
}

void perfaction_prim(void *in, typ type, void *out)
{
   profile_prim(in, type);
}


