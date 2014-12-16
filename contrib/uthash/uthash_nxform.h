/*
Copyright (c) 2003-2011, Troy D. Hanson     http://uthash.sourceforge.net
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*  Modified to lock bucket-by-bucket by ksaur */

#ifndef NXUTNXHASH_H
#define NXUTNXHASH_H 

#include <string.h>   /* memcmp,strlen */
#include <stddef.h>   /* ptrdiff_t */
#include <stdlib.h>   /* exit() */
#include <syscall.h> //TODO DBG
#include <pthread.h>

/* These macros use decltype or the earlier __typeof GNU extension.
   As decltype is only available in newer compilers (VS2010 or gcc 4.3+
   when compiling c++ source) this code uses whatever method is needed
   or, for VS2008 where neither is available, uses casting workarounds. */
#ifdef _MSC_VER         /* MS compiler */
#if _MSC_VER >= 1600 && defined(__cplusplus)  /* VS2010 or newer in C++ mode */
#define DECLTYPE(x) (decltype(x))
#else                   /* VS2008 or older (or VS2010 in C mode) */
#define NO_DECLTYPE
#define DECLTYPE(x)
#endif
#else                   /* GNU, Sun and other compilers */
#define DECLTYPE(x) (__typeof(x))
#endif

#ifdef NO_DECLTYPE
#define DECLTYPE_ASSIGN(dst,src)                                                 \
do {                                                                             \
  char **_da_dst = (char**)(&(dst));                                             \
  *_da_dst = (char*)(src);                                                       \
} while(0)
#else 
#define DECLTYPE_ASSIGN(dst,src)                                                 \
do {                                                                             \
  (dst) = DECLTYPE(dst)(src);                                                    \
} while(0)
#endif

/* a number of the hash function use uint32_t which isn't defined on win32 */
#ifdef _MSC_VER
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
#else
#include <inttypes.h>   /* uint32_t */
#endif

#define NXUTNXHASH_VERSION 1.9.4

#define uthash_fatal(msg) exit(-1)        /* fatal error (out of memory,etc) */
#define uthash_malloc(sz) malloc(sz)      /* malloc fcn                      */
#define uthash_free(ptr,sz) free(ptr)     /* free fcn                        */


/* initial number of buckets */
#define NXHASH_INITIAL_NUM_BUCKETS 32768      /* initial number of buckets        */
#define NXHASH_INITIAL_NUM_BUCKETS_LOG2 15  /* lg2 of initial number of buckets */
#define NXHASH_MANX_NUM_MUTENX 1024       /* max number of mutexes  (32 bytes each)   */
#define NXHASH_MANX_NUM_BUCKETS 16777216  /* 16 MB worth of buckets */
static pthread_rwlock_t * mutexes[NXHASH_INITIAL_NUM_BUCKETS]; // free mutexes fast...save ptrs to them.

/* calculate the element whose hash handle address is hhe */
#define ELMT_FROM_HH(tbl,hhp) ((void*)(((char*)(hhp)) - ((tbl)->hho)))

/* lock a SINGLE row/bucket of the hashtbl for READING ONLY*/
#define NXHASH_RDLOCK_RESV(head,findptr)                                        \
    NXHASH_RDLOCK_BUCKET(hh,head,findptr,sizeof(void *))

#define NXHASH_RDLOCK_BUCKET(hh,head,keyptr,keylen)                               \
do {                                                                             \
  unsigned _hf_bkt,_hf_hashv;                                                    \
     NXHASH_FCN(keyptr,keylen, (head)->hh.tbl->num_buckets, _hf_hashv, _hf_bkt);   \
/* printf("trying for bucket %d\n",_hf_bkt);                          */          \
     pthread_rwlock_rdlock(((head)->hh.tbl->buckets[ _hf_bkt ].bucket_mutex));    \
/* printf("Grabbed RD lock for [%d]\n", _hf_bkt);                           */    \
} while (0)

/* lock a SINGLE row/bucket of the hashtbl for WRITING ONLY*/
#define NXHASH_WRLOCK_RESV(head,findptr)                                        \
    NXHASH_WRLOCK_BUCKET(hh,head,findptr,sizeof(void *))

#define NXHASH_WRLOCK_BUCKET(hh,head,keyptr,keylen)                                \
do {                                                                              \
  unsigned _hf_bkt,_hf_hashv;                                                     \
     NXHASH_FCN(keyptr,keylen, (head)->hh.tbl->num_buckets, _hf_hashv, _hf_bkt);   \
 /*printf("trying for bucket %d\n", _hf_bkt);                                 */  \
     pthread_rwlock_wrlock(((head)->hh.tbl->buckets[ _hf_bkt ].bucket_mutex));    \
 /*printf("Grabbed WR lock for [%d]\n", _hf_bkt);*/                               \
} while (0)

/* UNlock a SINGLE row/bucket of the hashtbl*/
#define NXHASH_UNLOCK_RESV(head,findptr)                                      \
    NXHASH_UNLOCK_BUCKET(hh,head,findptr,sizeof(void *))

#define NXHASH_UNLOCK_BUCKET(hh,head,keyptr,keylen)                           \
do {                                                                             \
  unsigned _hf_bkt,_hf_hashv;                                                    \
     NXHASH_FCN(keyptr,keylen, (head)->hh.tbl->num_buckets, _hf_hashv, _hf_bkt);  \
     pthread_rwlock_unlock(((head)->hh.tbl->buckets[ _hf_bkt ].bucket_mutex));   \
 /*printf("RELEASED lock for [%d]\n", _hf_bkt);                             */   \
} while (0)


#define NXHASH_FIND(hh,head,keyptr,keylen,out)                                     \
do {                                                                             \
  unsigned _hf_bkt,_hf_hashv;                                                    \
  out=NULL;                                                                      \
  if (head) {                                                                    \
     NXHASH_FCN(keyptr,keylen, (head)->hh.tbl->num_buckets, _hf_hashv, _hf_bkt);   \
     if (NXHASH_BLOOM_TEST((head)->hh.tbl, _hf_hashv)) {                           \
       NXHASH_FIND_IN_BKT((head)->hh.tbl, hh, (head)->hh.tbl->buckets[ _hf_bkt ],  \
                        keyptr,keylen,out);                                      \
     }                                                                           \
  }                                                                              \
} while (0)

#ifdef NXHASH_BLOOM
#define NXHASH_BLOOM_BITLEN (1ULL << NXHASH_BLOOM)
#define NXHASH_BLOOM_BYTELEN (NXHASH_BLOOM_BITLEN/8) + ((NXHASH_BLOOM_BITLEN%8) ? 1:0)
#define NXHASH_BLOOM_MAKE(tbl)                                                     \
do {                                                                             \
  (tbl)->bloom_nbits = NXHASH_BLOOM;                                               \
  (tbl)->bloom_bv = (uint8_t*)uthash_malloc(NXHASH_BLOOM_BYTELEN);                 \
  if (!((tbl)->bloom_bv))  { uthash_fatal( "out of memory"); }                   \
  memset((tbl)->bloom_bv, 0, NXHASH_BLOOM_BYTELEN);                                \
  (tbl)->bloom_sig = NXHASH_BLOOM_SIGNATURE;                                       \
} while (0);

#define NXHASH_BLOOM_FREE(tbl)                                                     \
do {                                                                             \
  uthash_free((tbl)->bloom_bv, NXHASH_BLOOM_BYTELEN);                              \
} while (0);

#define NXHASH_BLOOM_BITSET(bv,idx) (bv[(idx)/8] |= (1U << ((idx)%8)))
#define NXHASH_BLOOM_BITTEST(bv,idx) (bv[(idx)/8] & (1U << ((idx)%8)))

#define NXHASH_BLOOM_ADD(tbl,hashv)                                                \
  NXHASH_BLOOM_BITSET((tbl)->bloom_bv, (hashv & (uint32_t)((1ULL << (tbl)->bloom_nbits) - 1)))

#define NXHASH_BLOOM_TEST(tbl,hashv)                                               \
  NXHASH_BLOOM_BITTEST((tbl)->bloom_bv, (hashv & (uint32_t)((1ULL << (tbl)->bloom_nbits) - 1)))

#else
#define NXHASH_BLOOM_MAKE(tbl) 
#define NXHASH_BLOOM_FREE(tbl) 
#define NXHASH_BLOOM_ADD(tbl,hashv) 
#define NXHASH_BLOOM_TEST(tbl,hashv) (1)
#endif

/************************* IMPORTANT ********************************************/
/*The original version of uthash.h did not need to be initialized.  However, as
 * this table is unaware of the underlying threads, with multi-threads it must
 * now be initialized by adding an empty entry before launching additional
 * threads.  There is probably a better way, but since the original code
 * initialized the table on first added entry, it was easy enough to just make
 * that a requirement rather than adding additional locks (tables regular locks
 * not yet initialized) to prevent the race condition of two threads trying to
 * init the tbl*/
/********************************************************************************/


/* Because the traditional iterator will be broken due to not maintaining TABLE
 * ptrs (from row to row), we must rely on the BUCKET ptrs to iterate through
 * and delete everything at the end (slower, but better than locking whole
 * tbl).  this function will free all malloced entries.  thd_num is the number
 * of the thread (ex:0-3), share is the number of threads (the number of ways
 * to split this)*/
#define FREE_ALL_MALLOCED_ENTRIES(hh, head, out, tmp,thd_num,share)        \
do {                                                                               \
  int i;                                                                           \
  for(i=(thd_num*(( ((head)->hh.tbl->num_buckets)  )/share)); i<((((head)->hh.tbl->num_buckets)/share)*(thd_num+1));i++){       \
    if (((head)->hh.tbl->buckets[i]).hh_head) {                                    \
        DECLTYPE_ASSIGN(out,ELMT_FROM_HH((head)->hh.tbl,((head)->hh.tbl->buckets[i]).hh_head)); \
     }                                                                             \
    else out=NULL;                                                                  \
    while (out) {                                                                   \
       tmp = out;                                                                   \
       if (out->hh.hh_next)                                                         \
           DECLTYPE_ASSIGN(out,ELMT_FROM_HH((head)->hh.tbl,out->hh.hh_next));       \
       else  out = NULL;                                                            \
       free(tmp);                                                                   \
    }                                                                               \
    free(mutexes[i]);								\
 }                                                                                  \
} while(0)



#define NXHASH_MAKE_TABLE(hh,head)                                                 \
do {                                                                               \
  (head)->hh.tbl = (NXUT_hash_table*)uthash_malloc(                                \
                  sizeof(NXUT_hash_table));                                        \
  if (!((head)->hh.tbl))  { uthash_fatal( "out of memory"); }                      \
  memset((head)->hh.tbl, 0, sizeof(NXUT_hash_table));                              \
  /*pthread_rwlock_init(&((head)->hh.link_lock), NULL);                           \
printf("linklock at %p\n", &((head)->hh.link_lock));*/                           \
  (head)->hh.tbl->tail = &((head)->hh);                                          \
  (head)->hh.tbl->num_buckets = NXHASH_INITIAL_NUM_BUCKETS;                        \
  (head)->hh.tbl->log2_num_buckets = NXHASH_INITIAL_NUM_BUCKETS_LOG2;              \
  (head)->hh.tbl->hho = (char*)(&(head)->hh) - (char*)(head);                    \
  (head)->hh.tbl->buckets = (NXUT_hash_bucket*)uthash_malloc(                      \
          NXHASH_INITIAL_NUM_BUCKETS*sizeof(struct NXUT_hash_bucket));               \
  if (! (head)->hh.tbl->buckets) { uthash_fatal( "out of memory"); }             \
  memset((head)->hh.tbl->buckets, 0,                                             \
          NXHASH_INITIAL_NUM_BUCKETS*sizeof(struct NXUT_hash_bucket));               \
  printf("INIT.  initializing %d bucket mutexes\n", ((head)->hh.tbl->num_buckets)); \
  int i;                                                                        \
  for(i=0; i<((head)->hh.tbl->num_buckets);i++){				\
     ((head)->hh.tbl->buckets[i].bucket_mutex) =                                \
           (pthread_rwlock_t*) malloc (sizeof(pthread_rwlock_t));               \
     pthread_rwlock_init(((head)->hh.tbl->buckets[i].bucket_mutex), NULL);	\
     mutexes[i] = ((head)->hh.tbl->buckets[i].bucket_mutex);                    \
  }										\
  NXHASH_BLOOM_MAKE((head)->hh.tbl);                                               \
  (head)->hh.tbl->signature = NXHASH_SIGNATURE;                                    \
} while(0)

#define NXHASH_ADD(hh,head,fieldname,keylen_in,add)                                \
        NXHASH_ADD_KEYPTR(hh,head,&add->fieldname,keylen_in,add)
 

#define NXHASH_ADD_KEYPTR(hh,head,keyptr,keylen_in,add)                            \
do {                                                                             \
 unsigned _ha_bkt;                                                               \
 (add)->hh.next = NULL;                                                          \
 (add)->hh.key = (char*)keyptr;                                                  \
 (add)->hh.keylen = keylen_in;                                                   \
 if (!(head)) { /*this case only for single-th, must be pre-init to avoid race*/ \
    head = (add);                                                                \
    (head)->hh.prev = NULL;                                                      \
    NXHASH_MAKE_TABLE(hh,head);                                                    \
   /* pthread_rwlock_wrlock(&((head)->hh.link_lock)); SLOW, locks whole tbl	*/\
 } else {                                                                        \
/*    pthread_rwlock_wrlock(&((head)->hh.link_lock));    	*/		 \
/*without uncommenting link_lock, the following 4 lines will probably not be accurate.*/\
    (head)->hh.tbl->tail->next = (add);                                          \
    (add)->hh.prev = ELMT_FROM_HH((head)->hh.tbl, (head)->hh.tbl->tail);         \
    (head)->hh.tbl->tail = &((add)->hh);                                         \
 }                                                                               \
 (head)->hh.tbl->num_items++;                                                    \
 (add)->hh.tbl = (head)->hh.tbl;                                                 \
 /*pthread_rwlock_unlock(&((head)->hh.link_lock));*/				 \
 NXHASH_FCN(keyptr,keylen_in, (head)->hh.tbl->num_buckets,                         \
         (add)->hh.hashv, _ha_bkt);                                              \
 NXHASH_ADD_TO_BKT((head)->hh.tbl->buckets[_ha_bkt],&(add)->hh);                   \
 NXHASH_BLOOM_ADD((head)->hh.tbl,(add)->hh.hashv);                                 \
 NXHASH_EMIT_KEY(hh,head,keyptr,keylen_in);                                        \
 NXHASH_FSCK(hh,head);                                                             \
} while(0)

#define NXHASH_TO_BKT( hashv, num_bkts, bkt )                                      \
do {                                                                             \
  bkt = ((hashv) & ((num_bkts) - 1));                                            \
} while(0)

/* delete "delptr" from the hash table.
 * "the usual" patch-up process for the app-order doubly-linked-list.
 * The use of _hd_hh_del below deserves special explanation.
 * These used to be expressed using (delptr) but that led to a bug
 * if someone used the same symbol for the head and deletee, like
 *  NXHASH_DELETE(hh,users,users);
 * We want that to work, but by changing the head (users) below
 * we were forfeiting our ability to further refer to the deletee (users)
 * in the patch-up process. Solution: use scratch space to
 * copy the deletee pointer, then the latter references are via that
 * scratch pointer rather than through the repointed (users) symbol.
 */
/* thread safe.....but must renable "link_lock" */
/*#define NXHASH_THR_DELETE(hh,head,delptr)                                              \
do {                                                                             \
    unsigned _hd_bkt;                                                            \
    struct NXUT_hash_handle *_hd_hh_del;                                           \
    if ( ((delptr)->hh.prev == NULL) && ((delptr)->hh.next == NULL) )  {         \
        uthash_free((head)->hh.tbl->buckets,                                     \
                    (head)->hh.tbl->num_buckets*sizeof(struct NXUT_hash_bucket) ); \
        NXHASH_BLOOM_FREE((head)->hh.tbl);                                         \
        uthash_free((head)->hh.tbl, sizeof(NXUT_hash_table));                      \
        head = NULL;                                                             \
       return;                                                                   \
    } else {                                                                     \
        pthread_rwlock_wrlock(&((head)->hh.link_lock));    		         \
        _hd_hh_del = &((delptr)->hh);                                            \
        if ((delptr) == ELMT_FROM_HH((head)->hh.tbl,(head)->hh.tbl->tail)) {     \
            (head)->hh.tbl->tail =                                               \
                (NXUT_hash_handle*)((char*)((delptr)->hh.prev) +                   \
                (head)->hh.tbl->hho);                                            \
        }                                                                        \
        if ((delptr)->hh.prev) {                                                 \
            ((NXUT_hash_handle*)((char*)((delptr)->hh.prev) +                      \
                    (head)->hh.tbl->hho))->next = (delptr)->hh.next;             \
        } else {                                                                 \
            DECLTYPE_ASSIGN(head,(delptr)->hh.next);                             \
        }                                                                        \
        if (_hd_hh_del->next) {                                                  \
            ((NXUT_hash_handle*)((char*)_hd_hh_del->next +                         \
                    (head)->hh.tbl->hho))->prev =                                \
                    _hd_hh_del->prev;                                            \
        }                                                                        \
        NXHASH_TO_BKT( _hd_hh_del->hashv, (head)->hh.tbl->num_buckets, _hd_bkt);  \
        pthread_rwlock_wrlock(((head)->hh.tbl->buckets[_hd_bkt].bucket_mutex));   \
        NXHASH_DEL_IN_BKT(hh,(head)->hh.tbl->buckets[_hd_bkt], _hd_hh_del);       \
        (head)->hh.tbl->num_items--;                                             \
        pthread_rwlock_unlock(((head)->hh.tbl->buckets[_hd_bkt].bucket_mutex));  \
        pthread_rwlock_unlock(&((head)->hh.link_lock));	                         \
    }                                                                            \
    NXHASH_FSCK(hh,head);                                                          \
} while (0)*/



/* convenience forms of NXHASH_FIND/NXHASH_ADD/NXHASH_DEL */
#define NXHASH_FIND_STR(head,findstr,out)                                          \
    NXHASH_FIND(hh,head,findstr,strlen(findstr),out)
#define NXHASH_ADD_STR(head,strfield,add)                                          \
    NXHASH_ADD(hh,head,strfield,strlen(add->strfield),add)
#define NXHASH_FIND_INT(head,findint,out)                                          \
    NXHASH_FIND(hh,head,findint,sizeof(int),out)
#define NXHASH_ADD_INT(head,intfield,add)                                          \
    NXHASH_ADD(hh,head,intfield,sizeof(int),add)
#define NXHASH_FIND_PTR(head,findptr,out)                                          \
    NXHASH_FIND(hh,head,findptr,sizeof(void *),out)
#define NXHASH_ADD_PTR(head,ptrfield,add)                                          \
    NXHASH_ADD(hh,head,ptrfield,sizeof(void *),add)
/*#define NXHASH_THR_DEL(head,delptr)                                                \
    NXHASH_THR_DELETE(hh,head,delptr) */

/* NXHASH_FSCK checks hash integrity on every add/delete when NXHASH_DEBUG is defined.
 * This is for uthash developer only; it compiles away if NXHASH_DEBUG isn't defined.
 */
#ifdef NXHASH_DEBUG
#define NXHASH_OOPS(...) do { fprintf(stderr,__VA_ARGS__); exit(-1); } while (0)
#define NXHASH_FSCK(hh,head)                                                       \
do {                                                                             \
    unsigned _bkt_i;                                                             \
    unsigned _count, _bkt_count;                                                 \
    char *_prev;                                                                 \
    struct NXUT_hash_handle *_thh;                                                 \
    if (head) {                                                                  \
        _count = 0;                                                              \
        for( _bkt_i = 0; _bkt_i < (head)->hh.tbl->num_buckets; _bkt_i++) {       \
            _bkt_count = 0;                                                      \
            _thh = (head)->hh.tbl->buckets[_bkt_i].hh_head;                      \
            _prev = NULL;                                                        \
            while (_thh) {                                                       \
               if (_prev != (char*)(_thh->hh_prev)) {                            \
                   NXHASH_OOPS("invalid hh_prev %p, actual %p\n",                  \
                    _thh->hh_prev, _prev );                                      \
               }                                                                 \
               _bkt_count++;                                                     \
               _prev = (char*)(_thh);                                            \
               _thh = _thh->hh_next;                                             \
            }                                                                    \
            _count += _bkt_count;                                                \
            if ((head)->hh.tbl->buckets[_bkt_i].count !=  _bkt_count) {          \
               NXHASH_OOPS("invalid bucket count %d, actual %d\n",                 \
                (head)->hh.tbl->buckets[_bkt_i].count, _bkt_count);              \
            }                                                                    \
        }                                                                        \
        if (_count != (head)->hh.tbl->num_items) {                               \
            NXHASH_OOPS("invalid hh item count %d, actual %d\n",                   \
                (head)->hh.tbl->num_items, _count );                             \
        }                                                                        \
        /* traverse hh in app order; check next/prev integrity, count */         \
        _count = 0;                                                              \
        _prev = NULL;                                                            \
        _thh =  &(head)->hh;                                                     \
        while (_thh) {                                                           \
           _count++;                                                             \
           if (_prev !=(char*)(_thh->prev)) {                                    \
              NXHASH_OOPS("invalid prev %p, actual %p\n",                          \
                    _thh->prev, _prev );                                         \
           }                                                                     \
           _prev = (char*)ELMT_FROM_HH((head)->hh.tbl, _thh);                    \
           _thh = ( _thh->next ?  (NXUT_hash_handle*)((char*)(_thh->next) +        \
                                  (head)->hh.tbl->hho) : NULL );                 \
        }                                                                        \
        if (_count != (head)->hh.tbl->num_items) {                               \
            NXHASH_OOPS("invalid app item count %d, actual %d\n",                  \
                (head)->hh.tbl->num_items, _count );                             \
        }                                                                        \
    }                                                                            \
} while (0)
#else
#define NXHASH_FSCK(hh,head) 
#endif

/* When compiled with -DNXHASH_EMIT_KEYS, length-prefixed keys are emitted to 
 * the descriptor to which this macro is defined for tuning the hash function.
 * The app can #include <unistd.h> to get the prototype for write(2). */
#ifdef NXHASH_EMIT_KEYS
#define NXHASH_EMIT_KEY(hh,head,keyptr,fieldlen)                                   \
do {                                                                             \
    unsigned _klen = fieldlen;                                                   \
    write(NXHASH_EMIT_KEYS, &_klen, sizeof(_klen));                                \
    write(NXHASH_EMIT_KEYS, keyptr, fieldlen);                                     \
} while (0)
#else 
#define NXHASH_EMIT_KEY(hh,head,keyptr,fieldlen)                    
#endif

/* default to Jenkin's hash unless overridden e.g. DNXHASH_FUNCTION=NXHASH_SANX */
#ifdef NXHASH_FUNCTION 
#define NXHASH_FCN NXHASH_FUNCTION
#else
#define NXHASH_FCN NXHASH_JEN
#endif

/* The Bernstein hash function, used in Perl prior to v5.6 */
#define NXHASH_BER(key,keylen,num_bkts,hashv,bkt)                                  \
do {                                                                             \
  unsigned _hb_keylen=keylen;                                                    \
  char *_hb_key=(char*)(key);                                                    \
  (hashv) = 0;                                                                   \
  while (_hb_keylen--)  { (hashv) = ((hashv) * 33) + *_hb_key++; }               \
  bkt = (hashv) & (num_bkts-1);                                                  \
} while (0)


/* SANX/FNV/OAT/JEN hash functions are macro variants of those listed at 
 * http://eternallyconfuzzled.com/tuts/algorithms/jsw_tut_hashing.aspx */
#define NXHASH_SANX(key,keylen,num_bkts,hashv,bkt)                                  \
do {                                                                             \
  unsigned _sx_i;                                                                \
  char *_hs_key=(char*)(key);                                                    \
  hashv = 0;                                                                     \
  for(_sx_i=0; _sx_i < keylen; _sx_i++)                                          \
      hashv ^= (hashv << 5) + (hashv >> 2) + _hs_key[_sx_i];                     \
  bkt = hashv & (num_bkts-1);                                                    \
} while (0)

#define NXHASH_FNV(key,keylen,num_bkts,hashv,bkt)                                  \
do {                                                                             \
  unsigned _fn_i;                                                                \
  char *_hf_key=(char*)(key);                                                    \
  hashv = 2166136261UL;                                                          \
  for(_fn_i=0; _fn_i < keylen; _fn_i++)                                          \
      hashv = (hashv * 16777619) ^ _hf_key[_fn_i];                               \
  bkt = hashv & (num_bkts-1);                                                    \
} while(0);
 
#define NXHASH_OAT(key,keylen,num_bkts,hashv,bkt)                                  \
do {                                                                             \
  unsigned _ho_i;                                                                \
  char *_ho_key=(char*)(key);                                                    \
  hashv = 0;                                                                     \
  for(_ho_i=0; _ho_i < keylen; _ho_i++) {                                        \
      hashv += _ho_key[_ho_i];                                                   \
      hashv += (hashv << 10);                                                    \
      hashv ^= (hashv >> 6);                                                     \
  }                                                                              \
  hashv += (hashv << 3);                                                         \
  hashv ^= (hashv >> 11);                                                        \
  hashv += (hashv << 15);                                                        \
  bkt = hashv & (num_bkts-1);                                                    \
} while(0)

#define NXHASH_JEN_MINX(a,b,c)                                                      \
do {                                                                             \
  a -= b; a -= c; a ^= ( c >> 13 );                                              \
  b -= c; b -= a; b ^= ( a << 8 );                                               \
  c -= a; c -= b; c ^= ( b >> 13 );                                              \
  a -= b; a -= c; a ^= ( c >> 12 );                                              \
  b -= c; b -= a; b ^= ( a << 16 );                                              \
  c -= a; c -= b; c ^= ( b >> 5 );                                               \
  a -= b; a -= c; a ^= ( c >> 3 );                                               \
  b -= c; b -= a; b ^= ( a << 10 );                                              \
  c -= a; c -= b; c ^= ( b >> 15 );                                              \
} while (0)

#define NXHASH_JEN(key,keylen,num_bkts,hashv,bkt)                                  \
do {                                                                             \
  unsigned _hj_i,_hj_j,_hj_k;                                                    \
  char *_hj_key=(char*)(key);                                                    \
  hashv = 0xfeedbeef;                                                            \
  _hj_i = _hj_j = 0x9e3779b9;                                                    \
  _hj_k = keylen;                                                                \
  while (_hj_k >= 12) {                                                          \
    _hj_i +=    (_hj_key[0] + ( (unsigned)_hj_key[1] << 8 )                      \
        + ( (unsigned)_hj_key[2] << 16 )                                         \
        + ( (unsigned)_hj_key[3] << 24 ) );                                      \
    _hj_j +=    (_hj_key[4] + ( (unsigned)_hj_key[5] << 8 )                      \
        + ( (unsigned)_hj_key[6] << 16 )                                         \
        + ( (unsigned)_hj_key[7] << 24 ) );                                      \
    hashv += (_hj_key[8] + ( (unsigned)_hj_key[9] << 8 )                         \
        + ( (unsigned)_hj_key[10] << 16 )                                        \
        + ( (unsigned)_hj_key[11] << 24 ) );                                     \
                                                                                 \
     NXHASH_JEN_MINX(_hj_i, _hj_j, hashv);                                          \
                                                                                 \
     _hj_key += 12;                                                              \
     _hj_k -= 12;                                                                \
  }                                                                              \
  hashv += keylen;                                                               \
  switch ( _hj_k ) {                                                             \
     case 11: hashv += ( (unsigned)_hj_key[10] << 24 );                          \
     case 10: hashv += ( (unsigned)_hj_key[9] << 16 );                           \
     case 9:  hashv += ( (unsigned)_hj_key[8] << 8 );                            \
     case 8:  _hj_j += ( (unsigned)_hj_key[7] << 24 );                           \
     case 7:  _hj_j += ( (unsigned)_hj_key[6] << 16 );                           \
     case 6:  _hj_j += ( (unsigned)_hj_key[5] << 8 );                            \
     case 5:  _hj_j += _hj_key[4];                                               \
     case 4:  _hj_i += ( (unsigned)_hj_key[3] << 24 );                           \
     case 3:  _hj_i += ( (unsigned)_hj_key[2] << 16 );                           \
     case 2:  _hj_i += ( (unsigned)_hj_key[1] << 8 );                            \
     case 1:  _hj_i += _hj_key[0];                                               \
  }                                                                              \
  NXHASH_JEN_MINX(_hj_i, _hj_j, hashv);                                             \
  bkt = hashv & (num_bkts-1);                                                    \
} while(0)

/* The Paul Hsieh hash function */
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__)             \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)             \
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif
#define NXHASH_SFH(key,keylen,num_bkts,hashv,bkt)                                  \
do {                                                                             \
  char *_sfh_key=(char*)(key);                                                   \
  uint32_t _sfh_tmp, _sfh_len = keylen;                                          \
                                                                                 \
  int _sfh_rem = _sfh_len & 3;                                                   \
  _sfh_len >>= 2;                                                                \
  hashv = 0xcafebabe;                                                            \
                                                                                 \
  /* Main loop */                                                                \
  for (;_sfh_len > 0; _sfh_len--) {                                              \
    hashv    += get16bits (_sfh_key);                                            \
    _sfh_tmp       = (get16bits (_sfh_key+2) << 11) ^ hashv;                     \
    hashv     = (hashv << 16) ^ _sfh_tmp;                                        \
    _sfh_key += 2*sizeof (uint16_t);                                             \
    hashv    += hashv >> 11;                                                     \
  }                                                                              \
                                                                                 \
  /* Handle end cases */                                                         \
  switch (_sfh_rem) {                                                            \
    case 3: hashv += get16bits (_sfh_key);                                       \
            hashv ^= hashv << 16;                                                \
            hashv ^= _sfh_key[sizeof (uint16_t)] << 18;                          \
            hashv += hashv >> 11;                                                \
            break;                                                               \
    case 2: hashv += get16bits (_sfh_key);                                       \
            hashv ^= hashv << 11;                                                \
            hashv += hashv >> 17;                                                \
            break;                                                               \
    case 1: hashv += *_sfh_key;                                                  \
            hashv ^= hashv << 10;                                                \
            hashv += hashv >> 1;                                                 \
  }                                                                              \
                                                                                 \
    /* Force "avalanching" of final 127 bits */                                  \
    hashv ^= hashv << 3;                                                         \
    hashv += hashv >> 5;                                                         \
    hashv ^= hashv << 4;                                                         \
    hashv += hashv >> 17;                                                        \
    hashv ^= hashv << 25;                                                        \
    hashv += hashv >> 6;                                                         \
    bkt = hashv & (num_bkts-1);                                                  \
} while(0);

#ifdef NXHASH_USING_NO_STRICT_ALIASING
/* The MurmurHash exploits some CPU's (x86,x86_64) tolerance for unaligned reads.
 * For other types of CPU's (e.g. Sparc) an unaligned read causes a bus error.
 * MurmurHash uses the faster approach only on CPU's where we know it's safe. 
 *
 * Note the preprocessor built-in defines can be emitted using:
 *
 *   gcc -m64 -dM -E - < /dev/null                  (on gcc)
 *   cc -## a.c (where a.c is a simple test file)   (Sun Studio)
 */
#if (defined(__i386__) || defined(__x86_64__)) 
#define MUR_GETBLOCK(p,i) p[i]
#else /* non intel */
#define MUR_PLUS0_ALIGNED(p) (((unsigned long)p & 0x3) == 0)
#define MUR_PLUS1_ALIGNED(p) (((unsigned long)p & 0x3) == 1)
#define MUR_PLUS2_ALIGNED(p) (((unsigned long)p & 0x3) == 2)
#define MUR_PLUS3_ALIGNED(p) (((unsigned long)p & 0x3) == 3)
#define WP(p) ((uint32_t*)((unsigned long)(p) & ~3UL))
#if (defined(__BIG_ENDIAN__) || defined(SPARC) || defined(__ppc__) || defined(__ppc64__))
#define MUR_THREE_ONE(p) ((((*WP(p))&0x00ffffff) << 8) | (((*(WP(p)+1))&0xff000000) >> 24))
#define MUR_TWO_TWO(p)   ((((*WP(p))&0x0000ffff) <<16) | (((*(WP(p)+1))&0xffff0000) >> 16))
#define MUR_ONE_THREE(p) ((((*WP(p))&0x000000ff) <<24) | (((*(WP(p)+1))&0xffffff00) >>  8))
#else /* assume little endian non-intel */
#define MUR_THREE_ONE(p) ((((*WP(p))&0xffffff00) >> 8) | (((*(WP(p)+1))&0x000000ff) << 24))
#define MUR_TWO_TWO(p)   ((((*WP(p))&0xffff0000) >>16) | (((*(WP(p)+1))&0x0000ffff) << 16))
#define MUR_ONE_THREE(p) ((((*WP(p))&0xff000000) >>24) | (((*(WP(p)+1))&0x00ffffff) <<  8))
#endif
#define MUR_GETBLOCK(p,i) (MUR_PLUS0_ALIGNED(p) ? ((p)[i]) :           \
                            (MUR_PLUS1_ALIGNED(p) ? MUR_THREE_ONE(p) : \
                             (MUR_PLUS2_ALIGNED(p) ? MUR_TWO_TWO(p) :  \
                                                      MUR_ONE_THREE(p))))
#endif
#define MUR_ROTL32(x,r) (((x) << (r)) | ((x) >> (32 - (r))))
#define MUR_FMINX(_h) \
do {                 \
  _h ^= _h >> 16;    \
  _h *= 0x85ebca6b;  \
  _h ^= _h >> 13;    \
  _h *= 0xc2b2ae35l; \
  _h ^= _h >> 16;    \
} while(0)

#define NXHASH_MUR(key,keylen,num_bkts,hashv,bkt)                        \
do {                                                                   \
  const uint8_t *_mur_data = (const uint8_t*)(key);                    \
  const int _mur_nblocks = (keylen) / 4;                               \
  uint32_t _mur_h1 = 0xf88D5353;                                       \
  uint32_t _mur_c1 = 0xcc9e2d51;                                       \
  uint32_t _mur_c2 = 0x1b873593;                                       \
  const uint32_t *_mur_blocks = (const uint32_t*)(_mur_data+_mur_nblocks*4); \
  int _mur_i;                                                          \
  for(_mur_i = -_mur_nblocks; _mur_i; _mur_i++) {                      \
    uint32_t _mur_k1 = MUR_GETBLOCK(_mur_blocks,_mur_i);               \
    _mur_k1 *= _mur_c1;                                                \
    _mur_k1 = MUR_ROTL32(_mur_k1,15);                                  \
    _mur_k1 *= _mur_c2;                                                \
                                                                       \
    _mur_h1 ^= _mur_k1;                                                \
    _mur_h1 = MUR_ROTL32(_mur_h1,13);                                  \
    _mur_h1 = _mur_h1*5+0xe6546b64;                                    \
  }                                                                    \
  const uint8_t *_mur_tail = (const uint8_t*)(_mur_data + _mur_nblocks*4); \
  uint32_t _mur_k1=0;                                                  \
  switch((keylen) & 3) {                                               \
    case 3: _mur_k1 ^= _mur_tail[2] << 16;                             \
    case 2: _mur_k1 ^= _mur_tail[1] << 8;                              \
    case 1: _mur_k1 ^= _mur_tail[0];                                   \
    _mur_k1 *= _mur_c1;                                                \
    _mur_k1 = MUR_ROTL32(_mur_k1,15);                                  \
    _mur_k1 *= _mur_c2;                                                \
    _mur_h1 ^= _mur_k1;                                                \
  }                                                                    \
  _mur_h1 ^= (keylen);                                                 \
  MUR_FMINX(_mur_h1);                                                   \
  hashv = _mur_h1;                                                     \
  bkt = hashv & (num_bkts-1);                                          \
} while(0)
#endif  /* NXHASH_USING_NO_STRICT_ALIASING */

/* key comparison function; return 0 if keys equal */
#define NXHASH_KEYCMP(a,b,len) memcmp(a,b,len) 

/* iterate over items in a known bucket to find desired item */
#define NXHASH_FIND_IN_BKT(tbl,hh,head,keyptr,keylen_in,out)                       \
do {                                                                             \
 if (head.hh_head) DECLTYPE_ASSIGN(out,ELMT_FROM_HH(tbl,head.hh_head));          \
 else out=NULL;                                                                  \
 while (out) {                                                                   \
    if (out->hh.keylen == keylen_in) {                                           \
        if ((NXHASH_KEYCMP(out->hh.key,keyptr,keylen_in)) == 0) break;             \
    }                                                                            \
    if (out->hh.hh_next) DECLTYPE_ASSIGN(out,ELMT_FROM_HH(tbl,out->hh.hh_next)); \
    else out = NULL;                                                             \
 }                                                                               \
} while(0)

/* add an item to a bucket  */
#define NXHASH_ADD_TO_BKT(head,addhh)                                              \
do {                                                                             \
 head.count++;                                                                   \
 (addhh)->hh_next = head.hh_head;                                                \
 (addhh)->hh_prev = NULL;                                                        \
 if (head.hh_head) { (head).hh_head->hh_prev = (addhh); }                        \
 (head).hh_head=addhh;                                                           \
} while(0)

/* remove an item from a given bucket */
#define NXHASH_DEL_IN_BKT(hh,head,hh_del)                                          \
    (head).count--;                                                              \
    if ((head).hh_head == hh_del) {                                              \
      (head).hh_head = hh_del->hh_next;                                          \
    }                                                                            \
    if (hh_del->hh_prev) {                                                       \
        hh_del->hh_prev->hh_next = hh_del->hh_next;                              \
    }                                                                            \
    if (hh_del->hh_next) {                                                       \
        hh_del->hh_next->hh_prev = hh_del->hh_prev;                              \
    }                                                                



/* This is an adaptation of Simon Tatham's O(n log(n)) mergesort */
/* Note that NXHASH_SORT assumes the hash handle name to be hh. 
 * NXHASH_SRT was added to allow the hash handle name to be passed in. */
#define NXHASH_SORT(head,cmpfcn) NXHASH_SRT(hh,head,cmpfcn)
#define NXHASH_SRT(hh,head,cmpfcn)                                                 \
do {                                                                             \
  unsigned _hs_i;                                                                \
  unsigned _hs_looping,_hs_nmerges,_hs_insize,_hs_psize,_hs_qsize;               \
  struct NXUT_hash_handle *_hs_p, *_hs_q, *_hs_e, *_hs_list, *_hs_tail;            \
  if (head) {                                                                    \
      _hs_insize = 1;                                                            \
      _hs_looping = 1;                                                           \
      _hs_list = &((head)->hh);                                                  \
      while (_hs_looping) {                                                      \
          _hs_p = _hs_list;                                                      \
          _hs_list = NULL;                                                       \
          _hs_tail = NULL;                                                       \
          _hs_nmerges = 0;                                                       \
          while (_hs_p) {                                                        \
              _hs_nmerges++;                                                     \
              _hs_q = _hs_p;                                                     \
              _hs_psize = 0;                                                     \
              for ( _hs_i = 0; _hs_i  < _hs_insize; _hs_i++ ) {                  \
                  _hs_psize++;                                                   \
                  _hs_q = (NXUT_hash_handle*)((_hs_q->next) ?                      \
                          ((void*)((char*)(_hs_q->next) +                        \
                          (head)->hh.tbl->hho)) : NULL);                         \
                  if (! (_hs_q) ) break;                                         \
              }                                                                  \
              _hs_qsize = _hs_insize;                                            \
              while ((_hs_psize > 0) || ((_hs_qsize > 0) && _hs_q )) {           \
                  if (_hs_psize == 0) {                                          \
                      _hs_e = _hs_q;                                             \
                      _hs_q = (NXUT_hash_handle*)((_hs_q->next) ?                  \
                              ((void*)((char*)(_hs_q->next) +                    \
                              (head)->hh.tbl->hho)) : NULL);                     \
                      _hs_qsize--;                                               \
                  } else if ( (_hs_qsize == 0) || !(_hs_q) ) {                   \
                      _hs_e = _hs_p;                                             \
                      _hs_p = (NXUT_hash_handle*)((_hs_p->next) ?                  \
                              ((void*)((char*)(_hs_p->next) +                    \
                              (head)->hh.tbl->hho)) : NULL);                     \
                      _hs_psize--;                                               \
                  } else if ((                                                   \
                      cmpfcn(DECLTYPE(head)(ELMT_FROM_HH((head)->hh.tbl,_hs_p)), \
                             DECLTYPE(head)(ELMT_FROM_HH((head)->hh.tbl,_hs_q))) \
                             ) <= 0) {                                           \
                      _hs_e = _hs_p;                                             \
                      _hs_p = (NXUT_hash_handle*)((_hs_p->next) ?                  \
                              ((void*)((char*)(_hs_p->next) +                    \
                              (head)->hh.tbl->hho)) : NULL);                     \
                      _hs_psize--;                                               \
                  } else {                                                       \
                      _hs_e = _hs_q;                                             \
                      _hs_q = (NXUT_hash_handle*)((_hs_q->next) ?                  \
                              ((void*)((char*)(_hs_q->next) +                    \
                              (head)->hh.tbl->hho)) : NULL);                     \
                      _hs_qsize--;                                               \
                  }                                                              \
                  if ( _hs_tail ) {                                              \
                      _hs_tail->next = ((_hs_e) ?                                \
                            ELMT_FROM_HH((head)->hh.tbl,_hs_e) : NULL);          \
                  } else {                                                       \
                      _hs_list = _hs_e;                                          \
                  }                                                              \
                  _hs_e->prev = ((_hs_tail) ?                                    \
                     ELMT_FROM_HH((head)->hh.tbl,_hs_tail) : NULL);              \
                  _hs_tail = _hs_e;                                              \
              }                                                                  \
              _hs_p = _hs_q;                                                     \
          }                                                                      \
          _hs_tail->next = NULL;                                                 \
          if ( _hs_nmerges <= 1 ) {                                              \
              _hs_looping=0;                                                     \
              (head)->hh.tbl->tail = _hs_tail;                                   \
              DECLTYPE_ASSIGN(head,ELMT_FROM_HH((head)->hh.tbl, _hs_list));      \
          }                                                                      \
          _hs_insize *= 2;                                                       \
      }                                                                          \
      NXHASH_FSCK(hh,head);                                                        \
 }                                                                               \
} while (0)

/* This function selects items from one hash into another hash. 
 * The end result is that the selected items have dual presence 
 * in both hashes. There is no copy of the items made; rather 
 * they are added into the new hash through a secondary hash 
 * hash handle that must be present in the structure. */
#define NXHASH_SELECT(hh_dst, dst, hh_src, src, cond)                              \
do {                                                                             \
  unsigned _src_bkt, _dst_bkt;                                                   \
  void *_last_elt=NULL, *_elt;                                                   \
  NXUT_hash_handle *_src_hh, *_dst_hh, *_last_elt_hh=NULL;                         \
  ptrdiff_t _dst_hho = ((char*)(&(dst)->hh_dst) - (char*)(dst));                 \
  if (src) {                                                                     \
    for(_src_bkt=0; _src_bkt < (src)->hh_src.tbl->num_buckets; _src_bkt++) {     \
      for(_src_hh = (src)->hh_src.tbl->buckets[_src_bkt].hh_head;                \
          _src_hh;                                                               \
          _src_hh = _src_hh->hh_next) {                                          \
          _elt = ELMT_FROM_HH((src)->hh_src.tbl, _src_hh);                       \
          if (cond(_elt)) {                                                      \
            _dst_hh = (NXUT_hash_handle*)(((char*)_elt) + _dst_hho);               \
            _dst_hh->key = _src_hh->key;                                         \
            _dst_hh->keylen = _src_hh->keylen;                                   \
            _dst_hh->hashv = _src_hh->hashv;                                     \
            _dst_hh->prev = _last_elt;                                           \
            _dst_hh->next = NULL;                                                \
            if (_last_elt_hh) { _last_elt_hh->next = _elt; }                     \
            if (!dst) {                                                          \
              DECLTYPE_ASSIGN(dst,_elt);                                         \
              NXHASH_MAKE_TABLE(hh_dst,dst);                                       \
            } else {                                                             \
              _dst_hh->tbl = (dst)->hh_dst.tbl;                                  \
            }                                                                    \
            NXHASH_TO_BKT(_dst_hh->hashv, _dst_hh->tbl->num_buckets, _dst_bkt);    \
            NXHASH_ADD_TO_BKT(_dst_hh->tbl->buckets[_dst_bkt],_dst_hh);            \
            (dst)->hh_dst.tbl->num_items++;                                      \
            _last_elt = _elt;                                                    \
            _last_elt_hh = _dst_hh;                                              \
          }                                                                      \
      }                                                                          \
    }                                                                            \
  }                                                                              \
  NXHASH_FSCK(hh_dst,dst);                                                         \
} while (0)

#define NXHASH_CLEAR(hh,head)                                                      \
do {                                                                             \
  if (head) {                                                                    \
    uthash_free((head)->hh.tbl->buckets,                                         \
                (head)->hh.tbl->num_buckets*sizeof(struct NXUT_hash_bucket));      \
    uthash_free((head)->hh.tbl, sizeof(NXUT_hash_table));                          \
    (head)=NULL;                                                                 \
  }                                                                              \
} while(0)

//DOES NOT WORK, hh.next/prev not maintained because don't want to lock whole tbl.
/*#ifdef NO_DECLTYPE
#define NXHASH_ITER(hh,head,el,tmp)                                                \
for((el)=(head), (*(char**)(&(tmp)))=(char*)((head)?(head)->hh.next:NULL);       \
  el; (el)=(tmp),(*(char**)(&(tmp)))=(char*)((tmp)?(tmp)->hh.next:NULL)) 
#else
#define NXHASH_ITER(hh,head,el,tmp)                                                \
for((el)=(head),(tmp)=DECLTYPE(el)((head)?(head)->hh.next:NULL);                 \
  el; (el)=(tmp),(tmp)=DECLTYPE(el)((tmp)?(tmp)->hh.next:NULL))
#endif*/

/* obtain a count of items in the hash */
#define NXHASH_COUNT(head) NXHASH_CNT(hh,head) 
#define NXHASH_CNT(hh,head) ((head)?((head)->hh.tbl->num_items):0)

typedef struct NXUT_hash_bucket {
   struct NXUT_hash_handle *hh_head;
   unsigned count;
   pthread_rwlock_t * bucket_mutex;

} NXUT_hash_bucket;

/* random signature used only to find hash tables in external analysis */
#define NXHASH_SIGNATURE 0xa0111fe1
#define NXHASH_BLOOM_SIGNATURE 0xb12220f2

typedef struct NXUT_hash_table {
   NXUT_hash_bucket *buckets;
   unsigned num_buckets, log2_num_buckets;
   unsigned num_items;
   struct NXUT_hash_handle *tail; /* tail hh in app order, for fast append    */
   ptrdiff_t hho; /* hash handle offset (byte pos of hash handle in element */

   /* in an ideal situation (all buckets used equally), no bucket would have
    * more than ceil(#items/#buckets) items. that's the ideal chain length. */
   unsigned ideal_chain_maxlen;

   /* nonideal_items is the number of items in the hash whose chain position
    * exceeds the ideal chain maxlen. these items pay the penalty for an uneven
    * hash distribution; reaching them in a chain traversal takes >ideal steps */
   unsigned nonideal_items;

   uint32_t signature; /* used only to find hash tables in external analysis */
#ifdef NXHASH_BLOOM
   uint32_t bloom_sig; /* used only to test bloom exists in external analysis */
   uint8_t *bloom_bv;
   char bloom_nbits;
#endif

} NXUT_hash_table;

typedef struct NXUT_hash_handle {
   struct NXUT_hash_table *tbl;
   void *prev;                       /* prev element in app order      */
   void *next;                       /* next element in app order      */
   struct NXUT_hash_handle *hh_prev;   /* previous hh in bucket order    */
   struct NXUT_hash_handle *hh_next;   /* next hh in bucket order        */
   void *key;                        /* ptr to enclosing struct's key  */
   unsigned keylen;                  /* enclosing struct's key len     */
   unsigned hashv;                   /* result of hash-fcn(key)        */
  /* pthread_rwlock_t link_lock;*/      /* locked when manipulating rows during add */
} NXUT_hash_handle;

#endif /* NXUTNXHASH_H */

