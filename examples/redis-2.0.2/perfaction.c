/* TEMPLATE FILE */
#include "perfaction.h"
#include "stdlib.h"
#include "stdio.h"
#include "ae.h"
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

void profwrite(int c, typ t, typ tm, int sz){

  fwrite(&c, sizeof(int), 1, prof);
  fwrite(&t, sizeof(long), 1, prof);
  fwrite(&tm, sizeof(long), 1, prof);
  fwrite(&sz, sizeof(int), 1, prof);
}

void perfaction_init(){ 
   //TODO deprecated
   //must init the table with something before the threads all race to do it
   type_count_entry *  entry = malloc(sizeof(type_count_entry));
   entry->t = -1;
   entry->count = -1;
   entry->isptr = 0;
   entry->size = 0;
   XHASH_ADD(hh, type_counts_head, t, sizeof(typ), entry);
}

void perfaction_free(){
  // TODO deprecated  
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

/**************** PROGRAM-SPECIFIC CODE ******************/

/* this handles the "void *ptr" in "struct robj".  The "void * ptr" is determined by fields "encoding" and "type".
   It's called from inside perfaction_struct.   

   I made it a separate function because for ser/deser, it needs to write/read the "encoding/type" fields first, this
  way you can call it wherever you need it rather than placing it at the top of perfaction_struct.
*/
void redis_item_union(struct redisObject *in, struct redisObject *out){
  /* nasty union handling */
  /* Tested with telnet localhost 6379 
     and then issuing these commands (SADD, LPUSH, etc)  http://redis.io/topics/data-types */
   if(!in) return;

   switch(in->type) {
     case REDIS_STRING:
        if(in->encoding == REDIS_ENCODING_INT){ 
           visit(&in->ptr, &out->ptr, TYPE_INT); 
        }
        else{
           visit(&in->ptr, &out->ptr, TYPE_sds); 
        }
        break;
     case REDIS_LIST:
       visit(&in->ptr, &out->ptr, TYPE_list_PTR_OF_TYPE_robj_PTR); 
       break;
     case REDIS_ZSET:
     case REDIS_HASH:
     case REDIS_SET:
       visit(&in->ptr, &out->ptr, TYPE_dict_PTR_OF_TYPE_robj_PTR__TYPE_robj_PTR); 
       break;
   }
}





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




int perfaction_ptr(void **in, typ type, void **out){
   prof_add_typ(type, 1, 0);
   return 1;
}
void perfaction_ptr_mapped(void **in, typ type, void **out){}
void perfaction_prim(void *in, typ type, void *out){
   //profwrite(prim, type, type);
   prof_add_typ(type, 0, 0);
}
int perfaction_struct(void *in, typ type, void *out){
   /* General  stuff for redis */
   if(type == TYPE_STRUCT_aeFileEvent || type == TYPE_aeFileEvent){
      struct aeFileEvent *ae = in;
      struct aeFileEvent *ae_o = out;
      visit(&ae->mask, TYPE_INT, &ae_o->mask);
      visit(&ae->rfileProc, TYPE_aeFileProc, &ae_o->rfileProc);
      visit(&ae->wfileProc, TYPE_aeFileProc, &ae_o->wfileProc);
      if(ae->mask != 0 && (ae->clientData!=NULL))
         visit(&ae->clientData, TYPE_redisClient_PTR, &ae_o->clientData);
 
      return 0;
   }
   if((type == TYPE_STRUCT_redisObject || type == TYPE_robj)){
      struct redisObject *robj = in;
      if(!in) return 0;
      /* call traverse on other fieldds in union */
      visit(&robj->type, TYPE_UNSIGNED_CHAR, &robj->type);
      visit(&robj->encoding, TYPE_UNSIGNED_CHAR, &robj->encoding);
      visit(&robj->storage, TYPE_UNSIGNED_CHAR, &robj->storage);
      visit(&robj->vtype, TYPE_UNSIGNED_CHAR, &robj->vtype);
      visit(&robj->refcount, TYPE_INT, &robj->refcount);
      visit(&robj->vm, TYPE_STRUCT_redisObjectVM, &robj->vm);
      redis_item_union(robj, robj);
      return 0;
   }

   prof_add_typ(type, 0, cstrider_get_size(type));

  return 1;
} 
