#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "profile.h"
#include <cstrider_api.h>

extern int TYPE_INT_i, TYPE_CHAR_i, TYPE_baz_PTR_i, TYPE_STRUCT__fptr_ll_PTR_i, TYPE_STRUCT__fptr_ll_i,
TYPE_STRUCT_list_PTR_i, TYPE_dummy_PTR_i,
TYPE_sds_i, TYPE_STRUCT__dummy_i;


struct traversal funs = {
   .perfaction_prim =&profile_prim, 
   .perfaction_struct = &profile_struct,
   .perfaction_ptr = &profile_ptr,
   .perfaction_ptr_mapped = &profile_ptr_mapped
};


#define NOTDEBUG
/////////////////////////// UNCOMMENT TO PROFILE.  Comment to test. TODO make this in the makefile or something...
#define FPTRLL
#define SDS
#define LIST1

int num_fptr_ll;
int num_dummy; //unique
int num_sds;
int num_int;
int num_list;
int num_ptr;
int num_char;
int num_fptr_ll_pointers;
int num_dummy_dups; 

void profile_init(){
  num_fptr_ll = 0;
  num_dummy = 0;
  num_sds = 0;
  num_int = 0;
  num_list = 0;
  num_ptr = 0;
  num_char = 0;
  num_fptr_ll_pointers = 0;
  num_dummy_dups = 0;
}
void profile_free(){
  printf("Number of num_fptr_ll = %d\n", num_fptr_ll);
  printf("Number of num_fptr_ll_pointers = %d\n", num_fptr_ll_pointers);
  printf("Number of num_dummy = %d\n", num_dummy);
  printf("Number of num_dummy_dups = %d\n", num_dummy_dups);
  printf("Number of num_sds = %d\n", num_sds);
  printf("Number of num_int = %d\n", num_int);
  printf("Number of num_list = %d\n", num_list);
  printf("Number of num_ptr = %d\n", num_ptr);
  printf("Number of num_char = %d\n", num_char);
}
extern void * map(int deref, int type, void *in, void *out);
struct kitsune_old_rename__baz {
  int len;
  struct _kitsune_old_rename__fptr_ll (*stupid);
};
struct kitsune_old_rename__fptr_ll {
  struct kitsune_old_rename__fptr_ll (*next);
  int (*foo_ptr)(int );
  int x;
};
int profile_ptr(void **in, typ type, void **out){

#ifdef NOTDEBUG
  num_ptr++;
  if(type == TYPE_baz_PTR_i){
    struct kitsune_old_rename__baz (*local_in) = *in;
    visit(&local_in->len, TYPE_INT_i, &local_in->len);
    visit(&local_in->stupid, TYPE_STRUCT__fptr_ll_PTR_i, &local_in->stupid);  
    return 0; //don't continue
  }
#ifdef LIST1
  if(cstrider_get_maintype(type) ==  TYPE_STRUCT_list_PTR_i)
    num_list++;
#endif

#ifdef FPTRLL
  if(type == TYPE_STRUCT__fptr_ll_PTR_i)
    num_fptr_ll_pointers++;  
#endif
#endif
  return 1;
}
void profile_ptr_mapped(void **in, typ type, void **out){

#ifdef NOTDEBUG
#ifdef FPTRLL
  if(type == TYPE_STRUCT__fptr_ll_PTR_i)
    num_fptr_ll_pointers++;  
#endif
  if(type == TYPE_dummy_PTR_i)
    num_dummy_dups++;
#endif
}
void profile_prim(void *in, typ type){
  if(type == TYPE_CHAR_i)
    num_char++;
  if(type == TYPE_INT_i)
    num_int++;
}
int profile_struct(void *in, typ type, void *out){

#ifdef NOTDEBUG
  if(type == TYPE_STRUCT__dummy_i)
    num_dummy++;
#ifdef FPTRLL
  if(type == TYPE_STRUCT__fptr_ll_i)
    num_fptr_ll++;  
#endif
#ifdef SDS
  if(type == TYPE_sds_i)
    num_sds++;
#endif
#endif

  return 1;
} 
