/* TEMPLATE FILE */
#include "perfaction.h"
#include "stdlib.h"
#include "stdio.h"


void perfaction_init(){ 
   //TODO deprecated
}

void perfaction_free(){
  // TODO deprecated  
  
}


/* action for a pointer discovery. 
 * return next place to visit. */
int perfaction_ptr(void **in, typ type, void **out){
   printf("Found type = %d\n", type);
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



