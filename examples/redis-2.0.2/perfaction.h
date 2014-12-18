#ifndef _PERFACTION_OPT
#define _PERFACTION_OPT
#include <sys/types.h> //off_t
#include "dsu.h"  //to get TYPE_whatever

typedef long typ;

/**** the user-implemented API ****/

//TODO first two are deprecated
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


/* program-specific*/
#define REDIS_STRING 0
#define REDIS_LIST 1
#define REDIS_SET 2
#define REDIS_ZSET 3
#define REDIS_HASH 4


#define REDIS_ENCODING_RAW 0 /* Raw representation */
#define REDIS_ENCODING_INT 1 /* Encoded as integer */
#define REDIS_ENCODING_ZIPMAP 2 /* Encoded as zipmap */
#define REDIS_ENCODING_HT 3 /* Encoded as an hash table */

struct redisObjectVM {
    off_t page;         /* the page at witch the object is stored on disk */
    off_t usedpages;    /* number of pages used on disk */
    time_t atime;       /* Last access time */
} vm;


/* The actual Redis Object */
typedef struct redisObject {
  void * ptr; /**DSU xfgen */
  //void *  ptr; /**DSU xfgen */
    unsigned char type;
    unsigned char encoding;
    unsigned char storage;  /* If this object is a key, where is the value?
                             * REDIS_VM_MEMORY, REDIS_VM_SWAPPED, ... */
    unsigned char vtype; /* If this object is a key, and value is swapped out,
                          * this is the type of the swapped out object. */
    int refcount;
    /* VM fields, this are only allocated if VM is active, otherwise the
     * object allocation function will just allocate
     * sizeof(redisObjct) minus sizeof(redisObjectVM), so using
     * Redis without VM active will not have any overhead. */
    struct redisObjectVM vm;
} robj;


#endif
