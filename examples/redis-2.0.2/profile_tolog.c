#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#include "perfaction_internal.h"
#include "../../../src/profile.h"
#include "dsu.h"



FILE * prof; 
pthread_mutex_t filewr_mutex = PTHREAD_MUTEX_INITIALIZER;

void profwrite(int c, long t, long tm){

  pthread_mutex_lock(&filewr_mutex);
  fwrite(&c, sizeof(int), 1, prof);
  fwrite(&t, sizeof(long), 1, prof);
  fwrite(&tm, sizeof(long), 1, prof);
  pthread_mutex_unlock(&filewr_mutex);
}

void profile_init(){

  prof = fopen("prof.txt", "wb"); 
}
void profile_free(){

   fclose(prof);
}
int ptr_wmain =1 , ptr =2,  prim=3, struct_wmain=4, structtype=5;

int profile_ptr(void **in, typ type, void **out){
 
   if(cstrider_get_ptrtype(type) != -1){
     typ ptrtype = cstrider_get_maintype(cstrider_get_ptrtype(type));
     profwrite(ptr_wmain, type, ptrtype);
   }else{
     profwrite(ptr, type, type);
   }
   return 1;
}
void profile_ptr_mapped(void **in, typ type, void **out){}
void profile_prim(void *in, typ type){
   profwrite(prim, type, type);
}
int profile_struct(void *in, typ type, void *out){

  if(cstrider_get_ptrtype(type) != -1){
    typ ptrtype = cstrider_get_maintype(type);
    profwrite(struct_wmain, type, ptrtype);
  }else{
    profwrite(structtype, type, type);
  }

  return 1;
} 
