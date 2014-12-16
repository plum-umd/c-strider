#ifndef _CSTRIDER_API_INTERNAL
#define _CSTRIDER_API_INTERNAL

#include "uthash_xform.h"
#include "uthash_nxform.h"


/* Type Table main structure  */
typedef struct
{
   typ type; 
   int arrlen; // number of elements in the array 
   typ arrbase; // base of the array 
   int size; // size of the struct if struct, or size of ptr. size of WHOLE array elem if array.
   void (*corresp_func_ptr)(void *arg_in, void *arg_out, typ gen);  //the xformer function, if any
   typ target_type;  // what THIS type points TO, else -1
   typ pointer_to;   // the pointer type for this type, if available 
                     // ^(ex: no unecessary ptrs to ptrs. but ptr to non-ptrs yes.*)
   int num_gen_args; // generics 
   typ *args;        // generics
   typ maintype;     // generics.  ex:maintype(TYPE_list_OF_TYPE_robj) returns  TYPE_list 
   typ out_type;     // corresponding 'out' type. same as 'type' for non-dsu cases
   XUT_hash_handle hh;// necessary for hashtable to operate
} type_hash_entry;

extern typ cstrider_out_type_internal(typ x);//TODO delete
/* generated code.... */
extern void build_gen_arrays(void);
extern void (*corresp_func_ptr[])(void *arg_in,void *arg_out, typ gen);

/* Internal API */ 
typedef struct
{
   void* key;
   void* addr;
   NXUT_hash_handle hh;
} pxform_mapping_entry;

void cstrider_type_init(void);
void cstrider_type_free(void);
void cstrider_visit_free(void);
void transform_map_rdlock(void *from);
void transform_map_wrlock(void *from);
void transform_map_unlock(void *from);
void populate_visit_all(void);
type_hash_entry * get_typeinfo_from_table(typ t);
pxform_mapping_entry * get_pxform_mappings_head();
void clear_pxform_mappings_head(void);

/* Move these here from _internal after fixing CIL */
//void kitsune_register_var(const char *var_name, const char *funcname, 
//                         const char *filename, const char *namespace,
//                         void* var_addr, size_t size, int auto_migrate);
//void *cstrider_get_symbol_addr_new(const char * key, const char * unused1, const char * unused2, const char * unused3);
//void *cstrider_get_symbol_addr_old(const char * key, const char * unused1, const char * unused2, const char * unused3);

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)+(uint32_t)(((const uint8_t *)(d))[0]) )
#endif
typ SuperFastHash (const char * data, int len);


void cstrider_type_init(void);
void cstrider_type_free(void);
void cstrider_visit_free(void);



#endif
