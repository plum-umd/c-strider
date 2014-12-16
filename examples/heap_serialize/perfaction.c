
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "serial.h"
#include "perfaction_internal.h"


/*TODO remove these */
void perfaction_init(){
}
void perfaction_free(){
}

/* mode set in commandline args*/
char mode;
int is_reading(){
   return mode == 'D';
}
int is_writing(){
   return mode == 'S';
}


/* action for a pointer discovery. 
 * return next place to visit. */
int perfaction_ptr(void **in, typ type, void **out){

   /*deserialization*/
   if(is_reading() ){
      return deserial_ptr(in, type, out);
   }
   /*serialization*/
   if(is_writing() ){
      return serial_ptr(in, type, out);
   }
   return 0;
}

/* action for a pointer re-discovery. 
 * Last parameter (ret_val)  is "returned".*/
void perfaction_ptr_mapped(void **in, typ type, void **out){

   /*deserialization*/
   if(is_reading() ){
      deserial_ptr_mapped(in, type, out);
   }
   /*serialization*/
   if(is_writing() ){
      serial_ptr_mapped(in, type, out);
   }
}

/* action for struct discovery */
int perfaction_struct(void *in, typ type, void *out){

   /*deserialization*/
   if(is_reading() ){
      return deserial_struct(in, type, out);
   }
   /*serialization*/
   if(is_writing() ){
      return serial_struct(in, type, out);
   }
   return 0;
}

void perfaction_prim(void *in, typ type, void *out){

   /*deserialization*/
   if(is_reading()){
      deserial_prim(in, type, out);
   }

   /*serialization*/
   if(is_writing()){
      serial_prim(in, type, out);
   }
}



