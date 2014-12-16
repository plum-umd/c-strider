
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "main.h"


//TODO move these
void perfaction_init(){}
void perfaction_free(){}

/* action for a pointer discovery. 
 * return next place to visit. */
int perfaction_ptr(void **in, typ type, void **out){
   if (type == TYPE_dbinfo_PTR_i){
      dbinfo *dbi = *in;
      printf("\tFound db struct at %p, (%c)\n", *in, dbi->p);
    }
   return 1;
}

/* action for a pointer re-discovery. 
 * Last parameter (ret_val)  is "returned".*/
void perfaction_ptr_mapped(void **in, typ type, void **out){

}

/* action for struct discovery */
int perfaction_struct(void *in, typ type, void *out){
   return 1;
}

void perfaction_prim(void *in, typ type, void *out){

}



