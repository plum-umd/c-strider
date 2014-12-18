
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "perfaction_internal.h"
#include "../../../src/update_internal.h"
#include "dsu.h"

extern void vmareas_init();
extern void vmareas_free();

void update_init(){
  vmareas_init();
}

void update_free(){
  vmareas_free();
}

void update_prim(void *in, typ type, void *out){
  if(in != out) 
     memcpy(out, in, cstrider_get_size(type));
}

int update_struct(void *in, typ type, void *out){
  return 1;
}

/* return value is 1 to continue the traversal (return 0 to abort the traversal) */
int update_ptr(void **in, typ type, void **out){

  void *lookup;
  /* pointer to null */
  if (*in == 0) {
    *out = 0; /* set out pointer to also be NULL */
    return 0; /* don't add mapping, don't contine */
  }
  char *symbol;
  if ((symbol = kitsune_lookup_addr_old(*in))) { /*look for stored mapping */
    kitsune_log("transform_ptr: pointer to non-heap data found [%s]", symbol);
    if (!(lookup = kitsune_lookup_key_new(symbol))) {
      kitsune_log("transform_ptr: no mapping found for %s\n", symbol);
      assert(0); /*we have a symbol that's not found in the new version. panic. */
    }
    *out = lookup; /* set out to the newly retrieved new symbol location.  Out is set to non-null so on returning, the traversal will map (*in, *out) where *out=lookup. */
    return 0; /* stop the traversal. this is complete. don't map */
  } else {
    if (cstrider_get_size(type) != cstrider_get_out_size(type)) {
      transform_delayedfree(*in, 0); /* free later, if parallel, to avoid nullptr problem*/
      *out = malloc(cstrider_get_out_size(type)); /*set out to newly malloced memory*/
    } else { /* use the same memory */
      *out = *in; /*set out to the same as in, we can reuse. (not necessarily the same to begin with) */
    }
  }
  return 1; /* continue the traversal. add a mapping */
}

void update_ptr_mapped(void **in, typ type, void **out){
}

