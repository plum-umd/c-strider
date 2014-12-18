#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#include "perfaction_internal.h"
#include "../../../src/profile.h"
#include "dsu.h"
#include "uthash_bgxform.h"



FILE * prof; 
typedef struct
{  
   typ t;
   int count;
   int isptr;
   int size;
   XUT_hash_handle hh;
} type_count_entry;
type_count_entry *type_counts_head = NULL; 

void prof_add_typ(typ t, int isptr, int sz){
   XHASH_WRLOCK_RESV(type_counts_head, &t);
   type_count_entry * entry;
   XHASH_FIND_PTR(type_counts_head, &t, entry);	
   if (!entry){
      entry = malloc(sizeof(type_count_entry));
      entry->t = t;
      entry->count = 1;
      entry->isptr = isptr;
      entry->size = sz;
      XHASH_ADD(hh, type_counts_head, t, sizeof(typ), entry);
   } else{
      (entry->count)++;
   }
   XHASH_UNLOCK_RESV(type_counts_head, &t);
}

void profwrite(int c, typ t, typ tm, int sz){

  fwrite(&c, sizeof(int), 1, prof);
  fwrite(&t, sizeof(long), 1, prof);
  fwrite(&tm, sizeof(long), 1, prof);
  fwrite(&sz, sizeof(int), 1, prof);
}


void profile_init(){
   //must init the table with something before the threads all race to do it
   type_count_entry *  entry = malloc(sizeof(type_count_entry));
   entry->t = -1;
   entry->count = -1;
   entry->isptr = 0;
   entry->size = 0;
   XHASH_ADD(hh, type_counts_head, t, sizeof(typ), entry);

}
void profile_free(){

   prof = fopen("prof.txt", "wb"); 
   type_count_entry *entry, *tmp; 
   XHASH_ITER(hh, type_counts_head, entry, tmp) {
       typ basetype;
       if(entry->t == -1){//this was the seed
         XHASH_DEL(type_counts_head,entry);  /* delete; users advances to next */
         free(entry);            /* optional- if you want to free  */
         continue;
       }
       else if(entry->isptr){
          if (cstrider_is_array(entry->t)){
             basetype = cstrider_get_array_base_type(cstrider_get_ptrtype(entry->t));
          } else if(cstrider_get_ptrtype(entry->t) != -1){
             basetype =cstrider_get_maintype(cstrider_get_ptrtype(entry->t));
          } else{
             basetype = cstrider_get_maintype(entry->t);
          }
       } else {
         if (cstrider_is_array(entry->t)){
             basetype = cstrider_get_array_base_type(entry->t);
         } else if(cstrider_get_ptrtype(entry->t) != -1){
             basetype = cstrider_get_maintype(entry->t);
         } else {
             basetype = cstrider_get_maintype(entry->t);
         }
       }
       profwrite(entry->count, entry->t, basetype, entry->size);
       XHASH_DEL(type_counts_head,entry);  
       free(entry); 
   }
   fclose(prof);
}

int profile_ptr(void **in, typ type, void **out){
   prof_add_typ(type, 1, 0);
   return 1;
}
void profile_ptr_mapped(void **in, typ type, void **out){}
void profile_prim(void *in, typ type){
   //profwrite(prim, type, type);
   prof_add_typ(type, 0, 0);
}
int profile_struct(void *in, typ type, void *out){

   prof_add_typ(type, 0, cstrider_get_size(type));

  return 1;
} 
