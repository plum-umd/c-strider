
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "main.h"
#include "serial.h"
#include "prog_specific.h"
#include <cstrider_api.h>


struct traversal deser_prog_funs = {
   .perfaction_prim =&deserial_prim,
   .perfaction_struct = &s_d_prog_struct,
   .perfaction_ptr = &deserial_ptr,
   .perfaction_ptr_mapped = &deserial_ptr_mapped
};
struct traversal ser_prog_funs = {
   .perfaction_prim =&serial_prim,
   .perfaction_struct = &s_d_prog_struct,
   .perfaction_ptr = &serial_ptr,
   .perfaction_ptr_mapped = &serial_ptr_mapped
};


/* current_service set in commandline args*/
extern int current_service;
int is_reading(){
   return current_service == DESERIALIZE;
}
int is_writing(){
   return current_service == SERIALIZE;
}



/* action for struct discovery */
int s_d_prog_struct(void *in, typ type, void *out)
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

