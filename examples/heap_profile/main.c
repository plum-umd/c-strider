#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <cstrider_api.h>




//////////////////////////// TESTS //////////////////////////////////////////

/**** passing tests ***/

/* basics */
#define SDS
#define SINGLE
#define SINGLEPTR
#define RHEAD
#define FPTRLL
#define SECONDS
#define QUEUE 
#define PTRPTR 


/* arrays */
#define ARRAY
#define ARRAY2
#define EDARR    
#define BUFFER   
#define VARI     
#define ARRAY3   

/* generics */
#define LIST1   
#define LIST2   
#define GEN1    
#define GEN2    

/* arrays of generics and nested generics */
#define REDIS

/* problem children */
///... no current known issues...yet :-P

///////////////////////////////////////////////////////////

#ifdef SECONDS
time_t seconds;
#endif


typedef struct _dummy{
    int * fun_ptr;
   char p;
}dummy;
char * c; //TODO this must be defind to make it build, todo fix.

#ifdef SDS
typedef char *sds;
sds test;
#endif

#ifdef ARRAY
dummy array[6];
#endif

#ifdef ARRAY2
dummy * array2[6];
#endif
#ifdef ARRAY3
int ** E_PTRARRAY(5) array3;
#endif

#ifdef SINGLE
dummy single;
#endif
#ifdef SINGLEPTR
dummy * single_ptr;
#endif
int i;

typedef struct _crummy{
    int * fun_ptr;
    int p;
}crummy;

#ifdef REDIS

typedef struct redisObject {
    char type;
    char encoding;
} robj;



struct dictEntry {
  void E_T(@k) * key; /** IMPORTANT: notice how i had to change the '*' to be AFTER E_T so that it's not opaqued.  for cstrider, we want to traverse these. I think. */
  void E_T(@v) * val; 
  struct dictEntry E_G(@k, @v) *next; /**DSU xfgen */
} E_GENERIC(@k, @v); /**DSU xfgen */
typedef struct dictEntry E_G(@k, @v) dictEntry E_GENERIC(@k, @v); /**DSU xfgen_ignore */


/* This is our hash table structure. Every dictionary has two of this as we
 * implement incremental rehashing, for the old to the new table. */
struct dictht {
    unsigned long size;
    dictEntry E_G(@k, @v) ** E_PTRARRAY(self.size) table; /**DSU xfgen */ /**DSU xfgen */
    unsigned long sizemask;
    unsigned long used;
} E_GENERIC(@k, @v); /**DSU xfgen */
typedef struct dictht E_G(@k, @v) dictht E_GENERIC(@k, @v); /**DSU xfgen_ignore */


struct dict {
  void * privdata; /**DSU xfgen */
  dictht E_G(@k, @v) ht[2]; /**DSU xfgen */
    int rehashidx; /* rehashing not in progress if rehashidx == -1 */
    int iterators; /* number of iterators currently running */
} E_GENERIC(@k, @v); /**DSU xfgen */
typedef struct dict E_G(@k, @v) dict E_GENERIC(@k, @v); /**DSU xfgen_ignore */

dict E_G(robj*,robj*) *mydict;

#endif


struct list {
  int dint;
  void E_T(@t0) *data0;
  void E_T(@t1) *data1;
  void * E_T(@t2) data2;
  struct list E_G(@t0,@t1,@t2) *next;
} E_GENERIC(@t0,@t1,@t2);
typedef struct list E_G(@t0,@t1,@t2) list E_GENERIC(@t0,@t1,@t2);



struct simplegen {
  int dint;
  void E_T(@t0) *fml;
} E_GENERIC(@t0);
typedef struct simplegen E_G(@t0) simplegen E_GENERIC(@t0);

#ifdef GEN1
simplegen E_G(char) *blah;
#endif
#ifdef GEN2
struct simplegen E_G(char*) *blah2;
#endif

#ifdef LIST1
struct list E_G(int, struct list<int,int,int>, [opaque]) *l;
#endif
#ifdef LIST2
list E_G(int,int,int) *l2 = NULL;
#endif





#ifdef VARI
typedef struct _vari{
    int len;
    crummy ** E_PTRARRAY(self.len) dumb_ptr;
}vari;
vari * v;
#endif


#ifdef RHEAD
typedef char * RULE_PTR;
typedef struct _rule_node_ {

  struct  _rule_node_ * rnNext;

  RULE_PTR rnRuleData;
  int iRuleNodeID;

}RULE_NODE;
RULE_NODE * rhead = NULL;
#endif

#ifdef EDARR
dummy ** E_PTRARRAY(7) darray;
dummy ** E_PTRARRAY(2) earray;
#endif
#ifdef PTRPTR
dummy ** ptrptr; //like in snort...
#endif

typedef int (*foo_ptr_t)( int );
#ifdef FPTRLL
int *h = NULL;
typedef struct _fptr_ll{
   struct _fptr_ll * next;
   foo_ptr_t foo_ptr;
   int x;
}fptr_ll;
fptr_ll * head = NULL;
typedef struct _baz {
  int len;
  struct _fptr_ll * stupid;
}baz;
baz * blag;
#endif

typedef struct
{
    dummy * d;

} tSfActionQueue;

#ifdef QUEUE
typedef tSfActionQueue* tSfActionQueueId;
tSfActionQueueId ihatesnort;
#endif

#ifdef BUFFER
typedef struct _buffer {
  int len;
  char * E_PTRARRAY(self.len) str;
}buffer;
buffer * buf;
#endif



void stupid(void){}
int x = 100;
int main(int argc, char **argv){

#ifdef QUEUE
    ihatesnort = malloc(sizeof(tSfActionQueue));
#endif
#ifdef SDS
    test = "silly";
#endif
    x = 106;
#ifdef REDIS
    //dict E_G(robj*,robj*) *dict;
    robj * ptr1 = malloc(sizeof(robj));
    robj * ptr2 = malloc(sizeof(robj));
    ptr1->type = 'f';
    ptr1->encoding = 'i';
    ptr2->type = 'g';
    ptr2->encoding = 'x';
    dictEntry * foo = malloc(sizeof(dictEntry));
    foo->key = ptr1;
    foo->val = ptr2;
    mydict = malloc(sizeof(dict));
    mydict->ht[1].size = 3;
    mydict->ht[1].table = malloc(sizeof(dictEntry*) *3);
    mydict->ht[1].table[1] = foo;
    
#endif
#ifdef BUFFER
    buf = malloc(sizeof(buffer));
    buf->len = 13;
    buf->str = malloc(buf->len);
    strncpy(buf->str, "NONULL_TERMIN", buf->len);
#endif
    dummy * d = malloc(sizeof(dummy));
#ifdef SINGLEPTR
    single_ptr = malloc(sizeof(dummy));
    single_ptr->fun_ptr = &x;
    single_ptr->p = 84;
#endif
#ifdef PTRPTR
    ptrptr = malloc(sizeof(dummy));
#endif

#ifdef EDARR
    darray = calloc(7, sizeof(dummy));
    earray = calloc(2, sizeof(dummy));
#endif
#ifdef ARRAY
    array[0].fun_ptr = &x;
    array[3].fun_ptr = &x;
#endif
#ifdef SINGLE
    single.fun_ptr = &x;
    single.p = 83;
    assert(single.fun_ptr == &x);
    printf("1single.fun_ptr = %p, &x = %p, *(void**)single.fun_ptr = %p\n",
    single.fun_ptr, &x, *(void**)single.fun_ptr);
    assert(single.fun_ptr == &x);
#endif
    d->fun_ptr = &x;
#ifdef ARRAY2
    array2[5] = d;
    printf("array2 at %p, &array2 at %p\n", array2, &array2);
#endif

#ifdef GEN1
      blah = malloc(sizeof(struct simplegen));
      blah->dint = 7;
      blah->fml = 'c';
#endif
#ifdef GEN2
      blah2 = malloc(sizeof(struct simplegen));
      blah2->dint = 77;
      blah2->fml = (char*)malloc(sizeof(char)*3);
      strncpy(blah2->fml, "FF",3);
#endif

#ifdef LIST1
      l = malloc(sizeof(struct list));
      l->dint = 5;
      l->data0 = 16;
      l->data1 = NULL;
      l->data2 = (void *)123; /* testing migration of opaque pointer */
      l->next = l;
#endif

#ifdef EDARR
      darray[0] = d;
      darray[1] = d;
      darray[2] = d;
      darray[3] = d;
      darray[4] = d;
      darray[5] = d;
      darray[6] = d;

      earray[0] = d;
      earray[1] = d;
#endif
#ifdef VARI
      v = malloc(sizeof(vari));
      v->len = 9;
      v->dumb_ptr = calloc(v->len, sizeof(crummy*));
      v->dumb_ptr[1] = malloc(sizeof(crummy));
      v->dumb_ptr[1]->p = 17;
#endif


#ifdef SECONDS
      seconds = 20132013;
#endif

#ifdef PTRPTR
      ptrptr[0] = d;
      printf("ptrptr[0]->fun_ptr = %p\n",ptrptr[0]->fun_ptr);
      assert(ptrptr[0]->fun_ptr == &x);
#endif
#ifdef ARRAY3
      array3 = calloc(5, sizeof(int*));
      printf("array3 at %p, &array3 at %p\n", array3, &array3);
      int hi = 555;
      array3[2] = &hi;
#endif
#ifdef FPTRLL
      h = malloc(sizeof(int));
      (*h) = 42;
      for (i = 0; i<5; i++){
        fptr_ll * n = calloc(1,sizeof(fptr_ll));
        n->x = 42;
        n->foo_ptr = &stupid;
        if (head == NULL)
        head = n;
        else{
           n->next=head;
	       head = n;
      }
    }
    fptr_ll * lala = malloc(sizeof(fptr_ll));
    lala->x = 9292;
    blag = malloc(sizeof(baz));
    blag->stupid = lala;
    blag->len = 1982;
#endif


#ifdef RHEAD
    for (i = 0; i<3; i++){
      RULE_NODE * n = calloc(1,sizeof(RULE_NODE));
      printf("n: %p\n", n);
      n->iRuleNodeID = 42;
      n->rnRuleData = malloc(16);
      printf("n->data: %p\n", n->rnRuleData);
      memset((n->rnRuleData),'@',16);
      assert(strncmp(n->rnRuleData,"@@@@@@@@@@@@@@@@",16)==0);
      if (rhead == NULL)
	    rhead = n;
      else{
        n->rnNext=rhead;
	    rhead = n;
      }
    }
    printf("rhead = %p\n", rhead);
#endif


  /* initialize the counters for profiling */
  profile_init(); 

  /* initialize and call the heap traversal */
  init(funs_init(), 1);
  visit_all();
  finish();

  /* print the counters */
  profile_free(); 
  
  return 0;
}
