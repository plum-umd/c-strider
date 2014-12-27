
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <cstrider_api.h>


struct traversal * funs;


/* action for a pointer discovery. 
 * return next place to visit. */
int perfaction_ptr(void **in, typ type, void **out){
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



struct traversal * funs_init(void){

   struct traversal * funs = malloc(sizeof(struct traversal));
   funs->perfaction_prim =&perfaction_prim; 
   funs->perfaction_struct = &perfaction_struct;
   funs->perfaction_ptr = &perfaction_ptr;
   funs->perfaction_ptr_mapped = &perfaction_ptr_mapped;
   return funs;
}
void funs_free(void){
   free(funs);
}
