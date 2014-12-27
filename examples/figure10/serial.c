#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "serial.h"
#include "perfaction_internal.h"
#include <cstrider_api.h>

//TODO check for EOF

extern int TYPE_OPAQUE_PTR_i, TYPE_CHAR_NT_i, TYPE_CHAR_NT_PTR_i;

FILE * ser; /* TODO parameterize*/


/* file accessors for perfaction.c if necessary */
void serial_internal_write(void * to_wr, int sz){
   fwrite(to_wr, sz, 1, ser);
}
void serial_internal_read(void * to_rd, int sz){
   fread(to_rd, sz, 1, ser);
}



struct traversal * ser_funs_init(void) {
   struct traversal * ser_funs = malloc(sizeof(struct traversal));
   ser_funs->perfaction_prim =&serial_prim; 
   ser_funs->perfaction_struct = &serial_struct;
   ser_funs->perfaction_ptr = &serial_ptr;
   ser_funs->perfaction_ptr_mapped = &serial_ptr_mapped;
   return ser_funs;
}
struct traversal * deser_funs_init(void) {
   struct traversal * deser_funs = malloc(sizeof(struct traversal));
   deser_funs->perfaction_prim =&deserial_prim; 
   deser_funs->perfaction_struct = &deserial_struct;
   deser_funs->perfaction_ptr = &deserial_ptr;
   deser_funs->perfaction_ptr_mapped = &deserial_ptr_mapped;
   return deser_funs;
}

/**************************************
    SERIALIZING 
 **************************************/

void serial_prim(void * in, typ type, void * out){

   int sz = cstrider_get_size(type);
   fwrite(in, sz, 1, ser);
}

/* write byte i*/
void serial_write_token(uintptr_t i){
   fwrite(&i, sizeof(uintptr_t), 1, ser);
}


/* records the lenght first, then the string*/
void serial_char_str(void * addr){

   int len;
   len = strlen(addr)+1;
   fwrite(&len, sizeof(int), 1, ser);  
   fwrite(addr, len, 1, ser);
}

int serial_ptr(void **in, typ type, void **out){

  if (*in == 0) {
    serial_write_token(0);
    return 0;
  }
  char *symbol;
  if ((symbol = lookup_addr(*in))) {
    uintptr_t two = 2;
    serial_prim(&two, TYPE_OPAQUE_PTR_i, 0);//record that we have been here by writing out the pointer
    serial_char_str(symbol);
    return 0;
    
  } else { 
    /*not a symbol */
    serial_prim(in, type, out);//record that we have been here by writing out the pointer

    /* it's a string.  write to disk right now */
    if(type ==  TYPE_CHAR_NT_PTR_i){ 
       serial_char_str(*in);
       return 0;
    } 
    /* not a string.  continue to whatever *in points to */
    else
       return 1;
  }

}

void serial_ptr_mapped(void **in, typ type, void **out){
   char * symbol;
   if ((symbol = lookup_addr(*in))) {
      serial_write_token((uintptr_t)SER_SYMBOL);
      serial_char_str(symbol);
   }
   else
      serial_prim(in, type, out);//record that we have been here.
}

int serial_struct(void *in, typ type, void *out){
  return 1;
}

/**************************************
    DESERIALIZING 
 **************************************/


void deserial_prim(void * in, typ type, void * out){

   int sz = cstrider_get_size(type);
   fread(out, sz, 1, ser); //changed this to be the "out" parameter 
}

/* records the lenght first, then the string*/
void deserial_char_str(void * addr){

   int len;
   fread(&len, sizeof(int), 1, ser); 
   char * tmp = malloc(len);
   fread(tmp, len, 1, ser);
   *(void**)addr = tmp;
}


/* deserialization of pointers is a bit differnt because there will be no data
 * at the end of the pointer, so *(void**)==0 will always be true. */
int deserial_ptr(void **in, typ type, void **out){

   void * present;
   void * tgt;
   /* reading*/
   fread(&present, sizeof(void*), 1, ser);
   if(present == (void*)SER_SYMBOL){
      char * symbol;
      deserial_char_str(&symbol);
      *out = lookup_key(symbol); //will write null if not found 
      free(symbol);
   }
   else if(present == NULL){ // null pointer
      *out = 0;
   }
   else if((tgt = find_mapping(present))) {
         *out = tgt;
   } 
   else { //haven't seen it ; read the data
      if(cstrider_get_ptrtype(type) == TYPE_CHAR_NT_i){ //string
         deserial_char_str(&tgt); //getting sz/malloc happens here.
         *out = tgt;
         add_mapping(present, tgt);
      } 
      else{
         typ next_type = cstrider_get_ptrtype(type);
         int to_alloc = cstrider_get_size(next_type);
         if(cstrider_is_array(next_type)){
             /* malloc the array itself (just the array...if it's an array of
             *  pointers pt'ed to gets malloced later) */
             to_alloc = cstrider_get_array_len(next_type);
         }
         tgt = malloc(to_alloc);
         add_mapping(present, tgt);
         *out = tgt;
         visit(tgt, next_type, tgt);
      }
   }
   return 0;
}


/*This will never get called, because "null" will never be mapped.... */
void deserial_ptr_mapped(void **in, typ type, void **out){
      printf("should be null, and should have been not mapped. instead was: %p\n", in);
      assert(0);
}

int deserial_struct(void *in, typ type, void *out){
  return 1;
}






