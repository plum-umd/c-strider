#ifndef _CSTRIDER_API
#define _CSTRIDER_API

#include <stdio.h>
#include <stdlib.h>
#include "perfaction_internal.h"

typedef long typ;
#define TYPE_FUNPTR 16

/***************************
 * Directing the traversal *
 ***************************/
void init(int service, int parallel);
void finish(void);
void register_root(void * in, typ t);
void deregister_root(void * in);
void visit_all(void);
extern void visit(void *in, typ type, void *out); /*transform.c*/

/*******************************************
 * Functions for querying the symbol table *
 *******************************************/
char *lookup_addr(void *addr);
void *lookup_key(const char *key);

/********************************************************
 * Functions for manipulating the pointer mapping table *
 ********************************************************/
void add_mapping(void *in, void *out);
void *find_mapping(void *in);



/***********************************************     
 * Functions for manipulating type information *
 ***********************************************/
/* returns true (1) if int, double, etc */
extern int cstrider_is_prim(typ t);
/* returns true if  pointer */
int cstrider_is_ptr(typ t);
/* returns true if function pointer */ /* generated */
extern int cstrider_is_funptr(typ t);
/* returns 1 if type x is a function pointer, else 0 */
int cstrider_is_array(typ t);
/* returns the size of the array (or else 0 if not array */
int cstrider_get_array_len(typ t); /*could possibly combine with the above*/
/* returns number of array elements */
int cstrider_get_num_array_elems(typ t);
/* returns size of the type x */
int cstrider_get_size(typ t);
/* same as above but for outsize in dsu case */
int cstrider_get_out_size(typ t);
/* For generics only...used to get the base type of generics.  ex:cstrider_get_maintype(TYPE_list_OF_TYPE_robj) returns  TYPE_list */
typ cstrider_get_maintype(typ t);
/* used to get the type of the pointer for type t.  Returns -1 if at the base (do not conflict with index 0 whic hsi TYPE_CHAR.) */
typ cstrider_get_ptrtype(typ t);
/* get the corresponding transformation function for type x */
void * cstrider_get_tvers_funptr(typ t);
/* get the type each element is of this array */
typ cstrider_get_array_base_type(typ t);
/* get the typ that points to this type, if any, else -1 */
typ cstrider_get_typ_ptstothis(typ t);


/*********************
 * Type Constructors *
 *********************/
typ mktypptr(typ type);
typ mktypfromgenargs(typ t, int numargs, typ *genargs);
typ mktyparr(int arrlen, typ type);

/************
 * Generics *
 ************/
typ * cstrider_get_generic_args(typ t);
typ cstrider_get_num_gen_args(typ t);
extern typ* get_args_arr(int i);
extern int get_num_gen_args_arr(int i);



/******************************************
 * Customizing the Traveral - Annotations *
 ******************************************/
#ifdef E_NOANNOT  //dsu.c
#define E_PTRARRAY(S)
#define E_ARRAY(S)
#define E_GENERIC(...)
#define E_G(...)
#define E_T(...)
#else
#define E_PTRARRAY(S) __attribute__((e_ptrarray(S)))
#define E_ARRAY(S) __attribute__((e_array(S)))
#define E_GENERIC(...) __attribute__((e_generic(#__VA_ARGS__)))
#define E_G(...) __attribute__((e_genuse(#__VA_ARGS__)))
#define E_T(...) __attribute__((e_type(#__VA_ARGS__)))
#endif


/* TODO move these to cstrider_api_internal.h after fixing CIL*/
void kitsune_register_var(const char *var_name, const char *funcname, 
                         const char *filename, const char *namespace,
                         void* var_addr, size_t size, int auto_migrate);
void *cstrider_get_symbol_addr_new(const char * key, const char * unused1, const char * unused2, const char * unused3);
void *cstrider_get_symbol_addr_old(const char * key, const char * unused1, const char * unused2, const char * unused3);

/* TODO delete these...after fixing CIL.*/
void stackvars_note_entry(const char *fun_name);
void stackvars_note_exit(const char *fun_name);
void stackvars_note_local(const char *name, void *addr, size_t size);
void stackvars_note_formal(const char *name, void *addr, size_t size);

#endif // _CSTRIDER_API
