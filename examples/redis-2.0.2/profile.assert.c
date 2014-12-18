#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "perfaction_internal.h"
#include "../../../src/profile.h"
#include "dsu.h"


void profile_init(){}
void profile_free(){}
int profile_ptr(void **in, typ type, void **out){
  return 1;
}
void profile_ptr_mapped(void **in, typ type, void **out){}
void profile_prim(void *in, typ type){
}
int profile_struct(void *in, typ type, void *out){

  return 1;

} 


