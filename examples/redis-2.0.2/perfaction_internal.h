#ifndef _PERFACTION_OPT
#define _PERFACTION_OPT

#include <inttypes.h>

typedef long typ;

/**** the user-implemented API ****/

/* called at the beginning. initialize stuff, open files, etc */
void perfaction_init(void);

/* called at the very end of traversal. free stuff, close files, etc */
void perfaction_free(void);

/* what to do with an int/char/etc */
void perfaction_prim(void *in, typ type, void *out);

/* return 1 to traverse inside struct. return 0 to not traverse inside struct */
int perfaction_struct(void *in, typ type, void *out);

/* return a pointer to continue traversal. return NULL to not traverse this branch further */
int perfaction_ptr(void **in, typ type, void **out);

/* what to do with an already visted heap item */
void perfaction_ptr_mapped(void **in, typ type, void **out);


/**** services ****/
/* used by perfaction to direct traffic */
extern int kitsune_is_writing(void);
extern int kitsune_is_reading(void);
extern int kitsune_is_profiling(void);
extern int kitsune_is_dsu(void);


extern char *kitsune_lookup_addr_old(void *addr);
extern char *kitsune_lookup_addr_new(void *addr);
extern void *kitsune_lookup_key_old(const char *key);
extern void *kitsune_lookup_key_new(const char *key);
extern void transform_add_mapping(void *from, void *to);
extern void *transform_find_mapping(void *from);

/****generated functions also available to the user to call in their functions. ***/
/* returns true if int, double, etc */
extern int cstrider_is_prim(typ t);
/* returns true if function pointer */
extern int cstrider_is_funptr(typ t);
/* returns 1 if type x is a function pointer, else 0 */
extern int cstrider_is_array(typ t);
/* returns the size of the array (or else 0 if not array */
extern int cstrider_get_array_len(typ t); /*could possibly combine with the above*/
/* returns size of the type x */
extern int cstrider_get_size(typ t);
/* same as above but for outsize in dsu case */
extern int cstrider_get_out_size(typ t);
/* called by programer to continue traversal manually */
extern void cstrider_traverse(void *in, void *out, typ type ); //TODO ordering....
/* For generics only...used to get the base type of generics.  ex:cstrider_get_maintype(TYPE_list_OF_TYPE_robj) returns  TYPE_list */
extern typ cstrider_get_maintype(typ t);
/* used to get the type of the pointer for type t.  Returns -1 if at the base (do not conflict with index 0 whic hsi TYPE_CHAR.) */
extern typ cstrider_get_ptrtype(typ t);
/* get the corresponding transformation function for type x */
extern void * cstrider_get_tvers_funptr(typ t);
/* get the type each element is of this array */
extern typ cstrider_get_array_base_type(typ t);
/* get the typ that points to this type, if any, else -1 */
extern typ cstrider_get_typ_ptstothis(typ t);



/*redis stuff shared by profile.c, perfactiion.c, etc */
#define REDIS_STRING 0
#define REDIS_LIST 1
#define REDIS_SET 2
#define REDIS_ZSET 3
#define REDIS_HASH 4


#define REDIS_ENCODING_RAW 0 /* Raw representation */
#define REDIS_ENCODING_INT 1 /* Encoded as integer */
#define REDIS_ENCODING_ZIPMAP 2 /* Encoded as zipmap */
#define REDIS_ENCODING_HT 3 /* Encoded as an hash table */

extern char* sdsnew(const char *init);

struct cstr_redisObjectVM {
    uint64_t page;         /* this should be off_t, which is an 8, not a 4 with redis flags (but can't find right flag...) */
    uint64_t usedpages;     
    time_t atime;       
} vm;

struct cstri_redisObject{
  void * ptr;
  unsigned char type;
  unsigned char encoding;
  unsigned char storage;  
  unsigned char vtype; 
  int refcount;
  struct cstr_redisObjectVM vm;
};

struct cstri_aeFileEvent{
    int mask; 
    void * rfileProc;
    void * wfileProc;
    void * clientData;
};

struct de {
  void * key;
  void * val;
  struct de *next;
}; 

/* kitsune internal stuff */
extern void kitsune_log(const char *fmt, ...);
extern void transform_delayedfree(void *addr, void * closure_args); //TODO better plan for this?
extern void cstrider_start_heaptraverse();
extern void cstrider_heaptraverse_teardown();
#endif

