#include "uthash.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "transform_internal.h"
#include "cstrider_api.h"
#include "cstrider_api_internal.h"
#include "uthash_nxform.h"
#include "uthash_xform.h"

/**********************
 * hashtable handles  *
 **********************/

/* XHASH. cstrider type table (can expand as necessary) */
type_hash_entry* type_hashtbl = NULL;

/* NXHASH, used for the main (in,out) address map
 * (does not expand, fast with multiple threads) */
pxform_mapping_entry *pxform_mappings_head = NULL;


/*******************
 * generated code  *
 *******************/
extern void cstrider_register_init(void);
extern int visit_all_len;
extern void (*visitalls[]);
extern int sizes_array_len;
extern int target_type_array[];
extern int ptsto_type_array[];
extern int sizes_array[];
extern int main_types_array[];
extern int TYPE_FUNPTR_i;


/**********************
 * C-strider Core API *
 **********************/
void init(int service, int parallel)
{
   cstrider_register_init();
   transform_init(parallel);
   launch_helpers();
}
void finish()
{
   kitsune_helpers_wait();
   transform_free();
}


/*****************
 * visit_all     *
 *****************/
typedef struct
{
   void *key;
   typ t;
   void (*fptr)();
   UT_hash_handle hh;
} el_t;

el_t *visit_all_hash = NULL;
el_t *visit_all_registered_hash = NULL;

/* read from the generated array of roots and populate the hashtbl */
void populate_visit_all()
{
   int i;
   for(i=0; i < visit_all_len; i++)
   {
      el_t *e = (el_t*)malloc(sizeof(el_t));
      e->key = visitalls[i];
      HASH_ADD_PTR(visit_all_hash,key,e);
   }
}

void visit_all()
{
   el_t *cur, *tmp;

   /* globals */
   HASH_ITER(hh, visit_all_hash, cur, tmp)
   {
      ((void (*)())(cur->key))();
   }
   /* user-registered */
   HASH_ITER(hh, visit_all_registered_hash, cur, tmp)
   {
      visit(cur->key, cur->t, cur->key);
   }

}

void register_root(void * in, typ t)
{
   el_t *e;
   HASH_FIND_PTR(visit_all_registered_hash, in, e);
   if(e) return;
   e = (el_t*)malloc(sizeof(el_t));
   e->key = in;
   e->t = t;
   HASH_ADD_PTR(visit_all_registered_hash,key,e);
}

void deregister_root(void * in)
{
   el_t *d;
   HASH_FIND_PTR(visit_all_registered_hash, &in, d);
   if(d)
   {
      HASH_DEL(visit_all_registered_hash,d);
      free(d);
   }
}

void cstrider_visit_free(void)
{
   el_t *current_user, *tmp;

   HASH_ITER(hh, visit_all_hash, current_user, tmp)
   {
      HASH_DEL(visit_all_hash, current_user);
      free(current_user);
   }
}

/***********************************************
 * Functions for manipulating type information *
 ***********************************************/

/*return 1 if array, else 0 */
int cstrider_is_array(typ t)
{
   type_hash_entry *te;
   te = get_typeinfo_from_table(t);
   if(!te)
   {
      printf("ERROR, cstrider_is_array, there is no type %lx\n", t);
      exit(-1);
   }
   return ((te->arrlen) != 0);
}

int cstrider_is_ptr(typ t)
{

   type_hash_entry *te;
   te = get_typeinfo_from_table(t);
   if(!te)
   {
      printf("ERROR, cstrider_get_tvers_funptr, there is no type %lx\n", t);
      exit(-1);
   }
   if((te->target_type != -1) || (te->type == TYPE_FUNPTR))
      return 1;
   return 0;
}

int cstrider_is_funptr(typ t)
{
   if((t==TYPE_FUNPTR_i) || (cstrider_get_tvers_funptr(t) == transform_fptr)) return 1;
   else return 0;
}


void * cstrider_get_tvers_funptr(typ t)
{
   type_hash_entry *te;
   te = get_typeinfo_from_table(t);
   if(!te)
   {
      printf("ERROR, cstrider_get_tvers_funptr, there is no type %lx\n", t);
      exit(-1);
   }
   return te->corresp_func_ptr;
}

typ cstrider_get_typ_ptstothis(typ t)
{
   type_hash_entry *te = get_typeinfo_from_table(t);
   if(!te)
   {
      printf("ERROR, cstrider_get_typ_ptstothis, there is no type %lx\n", t);
      exit(-1);
   }
   return te->pointer_to;
}

int cstrider_get_array_len(typ t)
{
   type_hash_entry *te;
   te = get_typeinfo_from_table(t);
   if(!te)
   {
      printf("ERROR, cstrider_get_array_len, there is no type %lx\n", t);
      exit(-1);
   }
   return ((te->arrlen* te->size));
}

int cstrider_get_num_array_elems(typ t)
{
   type_hash_entry *te;
   te = get_typeinfo_from_table(t);
   if(!te)
   {
      printf("ERROR, cstrider_get_num_array_elems, there is no type %lx\n", t);
      exit(-1);
   }
   return ((te->arrlen));
}

typ cstrider_get_maintype(typ t)
{
   type_hash_entry *te = get_typeinfo_from_table(t);
   if(!te)
   {
      printf("ERROR, cstrider_get_maintype, there is no type %lx\n", t);
      exit(-1);
   }
   return te->maintype;
}

typ cstrider_get_ptrtype(typ t)
{
   type_hash_entry *te = get_typeinfo_from_table(t);
   if(!te)
   {
      printf("ERROR, cstrider_get_ptrtype, there is no type %lx\\n", t);
      exit(-1);
   }
   return te->target_type;
}

typ cstrider_get_array_base_type(typ t)
{
   type_hash_entry *te = get_typeinfo_from_table(t);
   if(!te)
   {
      printf("ERROR, cstrider_get_size, there is no type %lx\\n", t);
      exit(-1);
   }
   return te->arrbase;
}

int cstrider_get_size(typ t)
{
   type_hash_entry *te = get_typeinfo_from_table(t);
   if(!te)
   {
      printf("ERROR, cstrider_get_size, there is no type %lx\\n", t);
      exit(-1);
   }
   return te->size;
}

int cstrider_get_out_size(typ t)
{
   type_hash_entry *te = get_typeinfo_from_table(t);
   if(!te)
   {
      printf("ERROR, cstrider_get_out_size, there is no type %lx\\n", t);
      exit(-1);
   }
   if (te->out_type != -1)
   {
      typ out = te->out_type;
      type_hash_entry *teo= get_typeinfo_from_table(out);
      if(!teo)
      {
         printf("ERROR, cstrider_get_out_size teo, there is no type %lx\\n", t);
         exit(-1);
      }
      return teo->size;
   }
   else
      return te->size;
}

typ * cstrider_get_generic_args(typ t)
{
   type_hash_entry *te = get_typeinfo_from_table(t);
   if(!te)
   {
      printf("ERROR, cstrider_get_generic_args, there is no type %lx\\n", t);
      exit(-1);
   }
   return te->args; /* 0 if none */
}

typ cstrider_get_num_gen_args(typ t)
{
   type_hash_entry *te = get_typeinfo_from_table(t);
   if(!te)
   {
      printf("ERROR, cstrider_get_num_generic_args, there is no type %lx\\n", t);
      exit(-1);
   }
   return te->num_gen_args; /* 0 if none...*/
}



/* internal */
type_hash_entry * get_typeinfo_from_table(typ t)
{
   type_hash_entry *te;
   XHASH_FIND(hh, type_hashtbl, &t, sizeof(typ), te);
   return te;
}

/* read from the arrays for the initial (non-constructed) set of types */
void cstrider_type_init(void)
{

   int i;
   type_hash_entry *to;

   /* populate generic arg arrays */
   build_gen_arrays();

   for(i=0; i<sizes_array_len; i++)
   {
      type_hash_entry *t;
      t = malloc(sizeof(type_hash_entry));
      t->type = i;
      t->arrlen = 0;
      t->arrbase = 0;
      t->corresp_func_ptr = corresp_func_ptr[i];
      t->target_type = target_type_array[i];
      t->pointer_to = ptsto_type_array[i];
      t->size = sizes_array[i];
      t->num_gen_args = get_num_gen_args_arr(i);
      t->args = get_args_arr(i);
      t->maintype = main_types_array[i];
      t->out_type = cstrider_out_type_internal(i);
      XHASH_ADD(hh, type_hashtbl, type, sizeof(typ), t);

      if(t->target_type != -1)
      {
         to = get_typeinfo_from_table(t->target_type);
         if(to)
            to->pointer_to = i;
         else
            ptsto_type_array[t->target_type] = t->type;
      }
   }
}

typedef struct _hashgencs
{
   typ t;
   int numargs;
   typ * genargs;
} hashgencs;

typ mktypfromgenargs(typ t, int numargs, typ *genargs)
{

   type_hash_entry *te;
   type_hash_entry *te_new;

   hashgencs h;
   h.t = t;
   h.numargs = numargs;
   h.genargs = genargs;
   typ combo = SuperFastHash((char *)&h, sizeof(hashgencs));

   XHASH_WRLOCK_RESV(type_hashtbl, &t);

   // check for existing
   te = get_typeinfo_from_table(combo);
   if(te)
   {
      XHASH_UNLOCK_RESV(type_hashtbl, &t);
      if(te->num_gen_args == numargs && te->args == genargs)
         return te->type;
   }

   // There's been no generic type with those args created. Make it.
   // Make sure that we find the base
   te = get_typeinfo_from_table(t); //t is the base
   if(!te)
   {
      printf("ERROR, mktypfromgenargs, there is no type %lx\n", t);
      exit(-1);
   }

   // Now create a new type that uses all but the args of 'typ t'
   te_new = malloc(sizeof(type_hash_entry));
   te_new->type = combo;  //This is a hash.
   te_new->out_type = te->type; //TODO
   te_new->arrlen = te->arrlen;
   te_new->arrbase = te->arrbase;
   te_new->corresp_func_ptr = te->corresp_func_ptr;
   if(te->target_type != -1)  //make pointer type too
   {
      te_new->target_type = mktypfromgenargs(te->target_type, numargs,genargs);
   }
   else
      te_new->target_type = te->target_type;
   te_new->pointer_to = te->pointer_to;
   te_new->size = te->size;
   te_new->num_gen_args = numargs; // generics constructed
   te_new->args = genargs;        // generics constructed
   te_new->maintype = t;
   XHASH_ADD(hh, type_hashtbl, type, sizeof(typ), te_new);
   XHASH_UNLOCK_RESV(type_hashtbl, &t);
   return te_new->type;

}


typ mktypptr(typ type)
{

   type_hash_entry *te_new;
   type_hash_entry *t;
   typ ptrto = SuperFastHash((char *)&type, sizeof(typ));

   XHASH_WRLOCK_RESV(type_hashtbl, &ptrto);

   // check for exisiting
   te_new = get_typeinfo_from_table(ptrto);
   if(te_new)
   {
      XHASH_UNLOCK_RESV(type_hashtbl, &ptrto);
      if(te_new->target_type == type)
         return te_new->type;
   }

   // the new
   te_new = malloc(sizeof(type_hash_entry));
   te_new->type = ptrto;
   te_new->arrlen = 0;
   te_new->arrbase = 0;
   te_new->corresp_func_ptr = transform_prim;
   te_new->target_type = type;
   te_new->pointer_to = 0;
   te_new->size = sizeof(void*);
   te_new->num_gen_args = 0;
   te_new->args = 0;
   te_new->maintype = ptrto;
   te_new->out_type = ptrto; //TODO del
   XHASH_ADD(hh, type_hashtbl, type, sizeof(typ), te_new);

   t = get_typeinfo_from_table(type);
   t->pointer_to = ptrto; //TODO, lock?

   XHASH_UNLOCK_RESV(type_hashtbl, &ptrto);
   return te_new->type;


}

typedef struct _hasharrcs
{
   int arrlen;
   typ t;
} hasharrcs;

typ mktyparr(int arrlen, typ type)
{

   type_hash_entry *te;
   type_hash_entry *te_new;
   hasharrcs h;
   h.t = type;
   h.arrlen = arrlen;
   typ combo = SuperFastHash((char *)&h, sizeof(hasharrcs));

   XHASH_WRLOCK_RESV(type_hashtbl, &type);

   // check for exisiting
   te = get_typeinfo_from_table(combo);
   if(te)
   {
      XHASH_UNLOCK_RESV(type_hashtbl, &type);
      if(te->arrlen == arrlen && te->arrbase == type)
         return te->type;
   }

   // if not, build new type based on 'typ type'
   te = get_typeinfo_from_table(type);
   if(!te)
   {
      printf("ERROR, mktyparr, there is no type %lx\n", type);
      exit(-1);
   }

   // the new
   te_new = malloc(sizeof(type_hash_entry));
   te_new->type =  combo;  //This is a hash.
   te_new->arrlen = arrlen;
   te_new->arrbase = type;

   // carved from the old
   te_new->corresp_func_ptr = te->corresp_func_ptr;
   te_new->target_type = te->target_type;
   te_new->pointer_to = te->pointer_to; //TODO testing
   te_new->size = te->size;//Note that this is the size of the individual
   // item. to get whole array size, multiply this field by arrlen
   te_new->num_gen_args = te->num_gen_args;
   te_new->args = te->args;
   te_new->maintype = te->maintype;
   te_new->out_type = te->out_type; //TODO delete
   XHASH_ADD(hh, type_hashtbl, type, sizeof(typ), te_new);
   XHASH_UNLOCK_RESV(type_hashtbl, &type);
   return te_new->type;
}


/************
 * symbol & *
 * mapping  *
 ************/
typedef int (*xform_fn_t)(void *);
typedef struct
{
   char *name;
   void *addr;
   size_t size;
   int auto_migrate;
   xform_fn_t xf_fn;
   UT_hash_handle hh_name;
   UT_hash_handle hh_addr;
   const char *var_name;
} hash_entry;
hash_entry* name_to_addr_hash = NULL;
hash_entry* addr_to_name_hash = NULL;

// TODO  doublecheck XHASH_ITER (vs HASH_ITER)...skipping any ends?
void cstrider_type_free(void)
{
   type_hash_entry *current_user, *tmp;

   XHASH_ITER(hh, type_hashtbl, current_user, tmp)
   {
      XHASH_DEL(type_hashtbl, current_user);  /* delete; users advances to next */
      free(current_user);            /* optional- if you want to free  */
   }
}

/* TODO cleanup kitsune CIL fields */
void * cstrider_get_symbol_addr_new(const char * key, const char * unused1, const char * unused2, const char * unused3)
{
   return cstrider_get_symbol_addr_old(key, 0,0,0);
}
void * cstrider_get_symbol_addr_old(const char * key, const char * unused1,
                                    const char * unused2, const char * unused3)
{

   hash_entry *entry;
   HASH_FIND(hh_name, name_to_addr_hash, key, strlen(key), entry);
   if(entry)
      return entry->addr;
   else
      return NULL;
}
char *lookup_addr(void *addr)
{
   hash_entry *entry;
   HASH_FIND(hh_addr, addr_to_name_hash, &addr, sizeof(addr), entry);
   if(entry)
      return entry->name;
   else
      return NULL;
}
void *lookup_key(const char *key)
{
   return cstrider_get_symbol_addr_new(key, 0,0,0);
}

/*****************
 * mapping table *
 *****************/

/* the parallel version of the hashmap implements its own thread saftey
 * (lock each row as necessary rather than whole tbl)
 */

pxform_mapping_entry * get_pxform_mappings_head()
{
   return pxform_mappings_head;
}
void clear_pxform_mappings_head()
{
   pxform_mappings_head = NULL;
}

/* this func used to be called only after transform_find_mapping, so we didn't
 * previously check for _find_mapping first.  However, with deserializing we
 * can't check before the value is read, this extra check is now necessary. */
void add_mapping(void *in, void *out)
{
   if(!in) return; //don't map a null pointer
   if (!find_mapping(in))
   {
      pxform_mapping_entry* new_entry = malloc(sizeof(pxform_mapping_entry));
      new_entry->key = in;
      new_entry->addr = out;
      NXHASH_ADD_PTR(pxform_mappings_head, key, new_entry);
   }
}

void *find_mapping(void *in)
{

   if(!in) return NULL;
   pxform_mapping_entry *entry;
   NXHASH_FIND_PTR(pxform_mappings_head, &in, entry);
   if (entry)
      return entry->addr;
   else
      return NULL;

}

void transform_map_rdlock(void *from)
{
   if(!from) return;
   if(transform_is_parallel())
      NXHASH_RDLOCK_RESV(pxform_mappings_head, &from);
}
void transform_map_wrlock(void *from)
{
   if(!from) return;
   if(transform_is_parallel())
   {
//      printf("thread %lu locking %p\n",(unsigned long)pthread_self(), from);
      NXHASH_WRLOCK_RESV(pxform_mappings_head, &from);
   }
}
void transform_map_unlock(void *from)
{
   if(!from) return;
   if(transform_is_parallel())
   {
      NXHASH_UNLOCK_RESV(pxform_mappings_head, &from);
      //    printf("thread %lu unlocked %p\n",(unsigned long)pthread_self(), from);
   }
}



/************
 * internal *
 * TODO mv  *
 ************/
/**
 * \ingroup internal
 * puts the address into the new hash address hash table
 */
static void cstrider_register_key(const char *key, void* var_addr,
                                  size_t size, int auto_migrate,
                                  const char *var_name)
{
   hash_entry* exists_key;
   hash_entry* exists_addr;

   HASH_FIND(hh_addr, addr_to_name_hash, &var_addr, sizeof(var_addr), exists_key);
   HASH_FIND(hh_name, name_to_addr_hash, key, strlen(key), exists_addr);

   if(exists_key || exists_addr)
   {
      /* For existing registrations, ensure that forward and backward mappings
       exist and match. */
      assert(exists_key);
      assert(exists_addr);
   }
   else
   {
      /* Allocate a new entry and add it to the forward and backward maps */
      hash_entry* new_entry = malloc(sizeof(hash_entry));
      new_entry->name = strdup(key);
      new_entry->addr = var_addr;
      new_entry->size = size;
      new_entry->auto_migrate = auto_migrate;
      new_entry->var_name = var_name;
      HASH_ADD_KEYPTR(hh_name, name_to_addr_hash, new_entry->name, strlen(new_entry->name), new_entry);
      HASH_ADD(hh_addr, addr_to_name_hash, addr, sizeof(var_addr), new_entry);
   }
}
/* TODO, this is a wrapper...for whatever reason, cil gets a Not_found error if
 * you remove this....  I have no idea how that's possible, since I removed all
 * occurances....but...here it is. */
void kitsune_register_var(const char *key, const char *unused1,
                          const char *unused2, const char *unused3,
                          void* var_addr, size_t size, int auto_migrate)
{
   cstrider_register_key(key, var_addr, size, auto_migrate, key);
}

// SuperFastHash from:
// http://www.azillionmonkeys.com/qed/hash.html
// Paul Hsieh
// LGPL 2.1 license.
typ SuperFastHash (const char * data, int len)
{
   uint32_t hash = len, tmp;
   int rem;

   if (len <= 0 || data == NULL) return 0;

   rem = len & 3;
   len >>= 2;

   /* Main loop */
   for (; len > 0; len--)
   {
      hash  += get16bits (data);
      tmp    = (get16bits (data+2) << 11) ^ hash;
      hash   = (hash << 16) ^ tmp;
      data  += 2*sizeof (uint16_t);
      hash  += hash >> 11;
   }

   /* Handle end cases */
   switch (rem)
   {
   case 3:
      hash += get16bits (data);
      hash ^= hash << 16;
      hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
      hash += hash >> 11;
      break;
   case 2:
      hash += get16bits (data);
      hash ^= hash << 11;
      hash += hash >> 17;
      break;
   case 1:
      hash += (signed char)*data;
      hash ^= hash << 10;
      hash += hash >> 1;
   }

   /* Force "avalanching" of final 127 bits */
   hash ^= hash << 3;
   hash += hash >> 5;
   hash ^= hash << 4;
   hash += hash >> 17;
   hash ^= hash << 25;
   hash += hash >> 6;

   return (typ)hash;//TODO, for 64-bit systems...find a different hash funct!
}
