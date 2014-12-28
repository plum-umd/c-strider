
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "main.h"
#include "serial.h"
#include "prog_specific.h"
#include <cstrider_api.h>


struct traversal union_funs = {
   .perfaction_prim =&union_prim,
   .perfaction_struct = &union_struct,
   .perfaction_ptr = &union_ptr,
   .perfaction_ptr_mapped = &union_ptr_mapped
};


/* current_service set in commandline args*/
extern int current_service;
int is_reading(){
   return current_service == DESERIALIZE;
}
int is_writing(){
   return current_service == SERIALIZE;
}


/* action for a pointer discovery.
 * return next place to visit. */
int union_ptr(void **in, typ type, void **out)
{

   /*deserialization*/
   if(is_reading() )
   {
      return deserial_ptr(in, type, out);
   }
   /*serialization*/
   if(is_writing() )
   {
      return serial_ptr(in, type, out);
   }
   return 0;
}

/* action for a pointer re-discovery.
 * Last parameter (ret_val)  is "returned".*/
void union_ptr_mapped(void **in, typ type, void **out)
{

   /*deserialization*/
   if(is_reading() )
   {
      deserial_ptr_mapped(in, type, out);
   }
   /*serialization*/
   if(is_writing() )
   {
      serial_ptr_mapped(in, type, out);
   }
}

/* action for struct discovery */
int union_struct(void *in, typ type, void *out)
{
   /* Service−agnostic, Program−specific */
   if (type == TYPE_STRUCT_tagged_union_i)
   {
      struct tagged_union *in_u = in ;
      struct tagged_union *out_u = out;
      visit (&in_u->tag, TYPE_INT_i, &out_u->tag);
      if(in_u->tag)
         visit(&in_u->u.x, TYPE_INT_i, &out_u->u.x);
      else
         visit(&in_u->u.c, TYPE_CHAR_i, &out_u->u.c);
      return 0;
   }

   /*deserialization*/
   if(is_reading() )
   {
      return deserial_struct(in, type, out);
   }
   /*serialization*/
   if(is_writing() )
   {
      return serial_struct(in, type, out);
   }
   return 0;
}

void union_prim(void *in, typ type, void *out)
{

   /*deserialization*/
   if(is_reading())
   {
      deserial_prim(in, type, out);
   }

   /*serialization*/
   if(is_writing())
   {
      serial_prim(in, type, out);
   }
}



