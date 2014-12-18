
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "perfaction_internal.h"
#include "../../../src/serial.h"
#include "../../../src/profile.h"
#include "../../../src/update_internal.h"
#include "dsu.h"
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

typedef struct cmultiState {
    int count;              /* Total number of MULTI commands */
    void * commands;     /* Array of MULTI commands */ /**DSU xfgen */
} cmultiState;

/* Time event structure */
typedef struct caeTimeEvent {
    long long id; /* time event identifier. */
    long when_sec; /* seconds */
    long when_ms; /* milliseconds */
    void *timeProc;
    void *finalizerProc;
    void *  clientData;
    struct caeTimeEvent *next;
} caeTimeEvent;

typedef struct cstr_redisClient {
    int fd;
    void *db;
    int dictid;
    char * qubuf;
    int argc, mbargc;
    void ** argv, ** mbargv; /**DSU xfgen */
    int bulklen;            /* bulk read len. -1 if not in bulk read mode */
    int multibulk;          /* multi bulk command format active */
    void *reply; /**DSU xfgen */
    int sentlen;
    time_t lastinteraction; /* time of the last interaction, used for timeout */
    int flags;              /* REDIS_SLAVE | REDIS_MONITOR | REDIS_MULTI ... */
    int slaveseldb;         /* slave selected db, if this client is a slave */
    int authenticated;      /* when requirepass is non-NULL */
    int replstate;          /* replication state if this is a slave */
    int repldbfd;           /* replication DB file descriptor */
    long repldboff;         /* replication DB file offset */
    off_t repldbsize;       /* replication DB file size */
    cmultiState mstate;      /* MULTI/EXEC state */
    int blockingkeysnum;    /* Number of blocking keys */
    void **  blockingkeys;    /* The key we are waiting to terminate a blocking*/ /**DSU xfgen */
                            /* operation such as BLPOP. Otherwise NULL. */
    time_t blockingto;      /* Blocking operation timeout. If UNIX current time
                             * is >= blockingto then the operation timed out. */
    void *io_keys;   /**DSU xfgen */       /* Keys this client is waiting to be loaded from the 
                             * swap file in order to continue. */
    void *pubsub_channels; /**DSU xfgen */
                            /* channels a client is interested in (SUBSCRIBE) */
     void *pubsub_patterns;   /**DSU xfgen */
                            /* patterns a client is interested in (SUBSCRIBE) */
} credisClient;

struct clistNode {
  struct clistNode  *prev; 
  struct clistNode  *next; 
  void * value; 
};


int fd_is_valid(int fd) {
  return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

int time_is_non_future(time_t t) {
  return t < time(NULL);
}

int asstcnt;

// These 2 functions could be here or in serial.c

// This is called by one of the signal handlers in redis.
// It will ser/deser just the database located in a field in the global.  (server.db)
// We don't want to serialize the entire server struct, just this field, so this function does that.
// The parameters are the pointer to (server.db) and the lenght field, as it's a variable len array
void serial_just_db(int dbnum, void *serverdb){
    cstrider_start_heaptraverse();
    void * db = *(void**)serverdb;
    cstrider_traverse(db, db, (mktyparr(dbnum, TYPE_redisDb)));
    cstrider_heaptraverse_teardown(); 
}

// Redis strings use custom allocators with information about string length.  The 
// actual "char *" data is a pointer into the middle of this structure.  
// Deserializing strings this way creates the "phantom" headers.
void deserial_sds_str(void * addr){
    int len;
    serial_internal_read(&len, sizeof(int)); 
    char * tmp = malloc(len);
    serial_internal_read(tmp, len);
    *(void**)addr = sdsnew(tmp);
    free(tmp);
}


void perfaction_init(){

   /*deserialization*/
   if(kitsune_is_reading()){
      deserial_init();
   }

   /*serialization*/
   if(kitsune_is_writing()){
      serial_init();
   }

   /*dsu*/
   if(kitsune_is_dsu()){
      update_init();
   }

   /*profiling*/
   if(kitsune_is_profiling()){
      asstcnt=0;
      profile_init();
   }
  
}

void perfaction_free(){
   
   /*deserialization*/
   if(kitsune_is_reading()){
      deserial_free();
   }

   /*serialization*/
   if(kitsune_is_writing()){
      serial_free();
   }

   /*dsu*/
   if(kitsune_is_dsu()){
      update_free();
   }

   /*profiling*/
   if(kitsune_is_profiling()){
      printf("asstcnt = %d\n", asstcnt);
      profile_free();
   }
  
}


/* action for a pointer discovery. 
 * return next place to visit. */
int perfaction_ptr(void **in, typ type, void **out){

   /*deserialization*/
   if(kitsune_is_reading() ){
      return deserial_ptr(in, type, out);
   }
   /*serialization*/
   if(kitsune_is_writing() ){
      return serial_ptr(in, type, out);
   }
   /* DSU */
   if(kitsune_is_dsu()){
      return update_ptr(in, type, out); 
   }
   /*profiling*/
   if(kitsune_is_profiling()){
      return profile_ptr(in, type, out);
   }
   return 0;
}

/* action for a pointer re-discovery. 
 * Last parameter (ret_val)  is "returned".*/
void perfaction_ptr_mapped(void **in, typ type, void **out){

   /*deserialization*/
   if(kitsune_is_reading() ){
      deserial_ptr_mapped(in, type, out);
   }
   /*serialization*/
   if(kitsune_is_writing() ){
      serial_ptr_mapped(in, type, out);
   }
   /*dsu*/
   if(kitsune_is_dsu()){
      update_ptr_mapped(in, type, out); 
   }
   /*profiling*/
   if(kitsune_is_profiling()){
      profile_ptr_mapped(in, type, out); 
   }

}


/* this handles the "void *ptr" in "struct robj".  The "void * ptr" is determined by fields "encoding" and "type".
   It's called from inside perfaction_struct.   

   I made it a separate function because for ser/deser, it needs to write/read the "encoding/type" fields first, this
  way you can call it wherever you need it rather than placing it at the top of perfaction_struct.
*/
void redis_item_union(struct cstri_redisObject *in, struct cstri_redisObject *out){
  /* nasty union handling */
  /* Tested with telnet localhost 6379 
     and then issuing these commands (SADD, LPUSH, etc)  http://redis.io/topics/data-types */
   if(!in) return;

   switch(in->type) {
     case REDIS_STRING:
        if(in->encoding == REDIS_ENCODING_INT){ 
           cstrider_traverse(&in->ptr, &out->ptr, TYPE_INT); 
        }
        else{
           cstrider_traverse(&in->ptr, &out->ptr, TYPE_sds); 
        }
        break;
     case REDIS_LIST:
       cstrider_traverse(&in->ptr, &out->ptr, TYPE_list_PTR_OF_TYPE_robj_PTR); 
       break;
     case REDIS_ZSET:
     case REDIS_HASH:
     case REDIS_SET:
       cstrider_traverse(&in->ptr, &out->ptr, TYPE_dict_PTR_OF_TYPE_robj_PTR__TYPE_robj_PTR); 
       break;
   }
}

/* action for struct discovery */
int perfaction_struct(void *in, typ type, void *out){

   /* General  stuff for redis */
   if(type == TYPE_STRUCT_aeFileEvent || type == TYPE_aeFileEvent){
      struct cstri_aeFileEvent *ae = in;
      struct cstri_aeFileEvent *ae_o = out;
      cstrider_traverse(&ae->mask, &ae_o->mask, TYPE_INT);
      cstrider_traverse(&ae->rfileProc, &ae_o->rfileProc, TYPE_aeFileProc);
      cstrider_traverse(&ae->wfileProc, &ae_o->wfileProc, TYPE_aeFileProc);
      if(ae->mask != 0 && (ae->clientData!=NULL))
         cstrider_traverse(&ae->clientData, &ae_o->clientData, TYPE_redisClient_PTR);
 
      return 0;
   }

   /*serialization*/
   if(kitsune_is_reading() ){
      if((type == TYPE_STRUCT_redisObject || type == TYPE_robj)){ 
        if(!in) return 0;
        struct cstri_redisObject *robj = in;
        struct cstr_redisObjectVM vm = robj->vm;
	/* these are the other fields in the robj union.  I am calling "read"
         * on them directly rather than 'cstrider_traverse' because it's faster this
         * way, especially for "vm.page/vm.usedpages", etc....the "vm.'s" have about 4 levels
         * of typedefs that must all be resolved/travered, wherewas here I can just
         * label them with the end-type directly*/
        serial_internal_read(&robj->type,  sizeof(unsigned char));
        serial_internal_read(&robj->encoding, sizeof(unsigned char));
        serial_internal_read(&robj->storage,  sizeof(unsigned char));
        serial_internal_read(&robj->vtype,  sizeof(unsigned char));
        serial_internal_read(&robj->refcount, sizeof(int));
        serial_internal_read(&vm.page, sizeof(uint64_t)); 
        serial_internal_read(&vm.usedpages, sizeof(uint64_t));
        serial_internal_read(&vm.atime, sizeof(time_t));
        if((robj->type == REDIS_STRING) && (robj->encoding != REDIS_ENCODING_INT))
            deserial_sds_str(&robj->ptr); //this is unique to deserial
        else
            redis_item_union(robj, robj); //else can handle "generally"
        return 0;
      } 
      return deserial_struct(in, type, out);
   }
   /*deserialization*/
   if(kitsune_is_writing() ){
      if((type == TYPE_STRUCT_redisObject || type == TYPE_robj)){ 
        if(!in) return 0;
        struct cstri_redisObject *robj = in;
        struct cstr_redisObjectVM vm = robj->vm;
        /*see commens in deser above*/
        serial_internal_write(&robj->type,  sizeof(unsigned char));
        serial_internal_write(&robj->encoding, sizeof(unsigned char));
        serial_internal_write(&robj->storage,  sizeof(unsigned char));
        serial_internal_write(&robj->vtype,  sizeof(unsigned char));
        serial_internal_write(&robj->refcount, sizeof(int));
        serial_internal_write(&vm.page, sizeof(uint64_t)); 
        serial_internal_write(&vm.usedpages, sizeof(uint64_t));
        serial_internal_write(&vm.atime, sizeof(time_t));
        if((robj->type == REDIS_STRING) && (robj->encoding != REDIS_ENCODING_INT))
            serial_char_str(robj->ptr); 
        else
            redis_item_union(robj, robj);
        return 0;
     }
     return serial_struct(in, type, out);
   }


   /*dsu*/
   if(kitsune_is_dsu()){

      /* Replicating typedef dictType -> typedef dictType: {} xf rule from
       * original kitsune paper.  */
      if(cstrider_get_maintype(type) == TYPE_STRUCT_dictType)
         return 0;
      /* opaques.  these wouldn't even be _generated_ in original kitsune because they are never traversed in dsu */
      if(type == TYPE_STRUCT_redisObjectVM || type ==  TYPE_LONG_LONG || type == TYPE_INT || type == TYPE_LONG || type == TYPE_time_t || type == TYPE_aeFiredEvent ){
         return 0;
      }

      /* Replicating opaquing the pointer in dict.h (void * E_T(@k) key intead of void E_T(@k) * key;) in original kit paper */
      if(type == TYPE_STRUCT_dictEntry || cstrider_get_maintype(type) == TYPE_STRUCT_dictEntry){
         struct de * in_de = in;
         struct de * out_de = out;
         perfaction_prim(&in_de->key, TYPE_OPAQUE_PTR,  &out_de->key);
         perfaction_prim(&in_de->val, TYPE_OPAQUE_PTR,  &out_de->val);
         cstrider_traverse(&in_de->next, &out_de->next, mktypfromgenargs(TYPE_STRUCT_dictEntry_PTR, 2, cstrider_get_generic_args(type)));
         return 0;
      }


      if((type == TYPE_STRUCT_redisObject || type == TYPE_robj)){
         /*dsu optimize */
         if(in == out)
            return 0;
         else{
            struct cstri_redisObject *robj = in;
            struct cstri_redisObject *robj_o = out;
            redis_item_union(robj, robj_o);
            return 0;
         }
     }
     return update_struct(in, type, out); 
   }

   if(kitsune_is_profiling()){
      if(!in) return 0;
      /* opaques.  these wouldn't even be _generated_ in original kitsune because they are never traversed in dsu */
      if(type == TYPE_STRUCT_redisObjectVM || type ==  TYPE_LONG_LONG || type == TYPE_INT || type == TYPE_LONG || type == TYPE_time_t || type == TYPE_aeFiredEvent ){
         return 0;
      }

      /* Replicating opaquing the pointer in dict.h (void * E_T(@k) key intead of void E_T(@k) * key;) in original kit paper */
      if(type == TYPE_STRUCT_dictEntry || cstrider_get_maintype(type) == TYPE_STRUCT_dictEntry){
         struct de * in_de = in;
         struct de * out_de = out;
         perfaction_prim(&in_de->key, TYPE_OPAQUE_PTR,  &out_de->key);
         perfaction_prim(&in_de->val, TYPE_OPAQUE_PTR,  &out_de->val);
         cstrider_traverse(&in_de->next, &out_de->next, mktypfromgenargs(TYPE_STRUCT_dictEntry_PTR, 2, cstrider_get_generic_args(type)));
         return 0;
      }
      if((type == TYPE_STRUCT_redisObject || type == TYPE_robj)){
         struct cstri_redisObject *robj = in;
         /* call traverse on other fieldds in union */
         assert(robj->refcount >=0);
         assert(robj->type == REDIS_STRING || robj->type == REDIS_LIST || robj->type == REDIS_SET || robj->type == REDIS_ZSET || robj->type == REDIS_HASH);
         //printf("asserted robj\n");
         asstcnt++;
         redis_item_union(robj, robj);
         return 0;
     }

     if(type == TYPE_redisClient || type == TYPE_STRUCT_redisClient){
        struct cstr_redisClient * c = in;
        assert(fd_is_valid(c->fd));
        assert(time_is_non_future(c->lastinteraction));
//        printf("asserted file descriptor\n");
        asstcnt++;
        return 0;
     } 
     if(type ==  TYPE_STRUCT_listNode || type == TYPE_listNode){
        struct clistNode * c = in;
        if(c->prev){
          assert(c->prev->next == c);
          //printf("asserted Next node in doubly-linked list has prev");
          asstcnt++;
        }
        return 0;
     }

     if((type ==  TYPE_STRUCT_aeTimeEvent) || (type ==  TYPE_aeTimeEvent)){
        struct caeTimeEvent * te = in;
        assert(te->timeProc != NULL);
       // printf("asserted time event\n");
        asstcnt++;
        return 0;
     }
     return profile_struct(in, type, out);
   }

   //not prof/upd/ser/deser
   return 0;
}


void perfaction_prim(void *in, typ type, void *out){

   /*deserialization*/
   if(kitsune_is_reading()){
      deserial_prim(in, type, out);
   }

   /*serialization*/
   if(kitsune_is_writing()){
      serial_prim(in, type, out);
   }

   /*dsu*/
   if(kitsune_is_dsu()){
      update_prim(in, type, out);
   }

   /*profiling*/
   if(kitsune_is_profiling()){
      profile_prim(in, type);
   }
}


