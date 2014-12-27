#include <sys/time.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "uthash.h" //used for thread-local data (does not need to be thread safe)


#include "queue.h"
#include "transform_internal.h"
//#include "perfaction_internal.h"
#include "cstrider_api.h"
#include "cstrider_api_internal.h"

//// TODO deprecated...fix these...


/* Parallel stuff */
static int is_parallel = 0;
#define NUM_THREADS  4  /*this includes the main thread also.  will launch NUM_THREADS-1 helper threads.*/
#define SPLIT 4 /* split arrays into 4 parts - must be even #*/
pthread_t tid[NUM_THREADS -1]; /*store helper threads*/
queue queue_array[NUM_THREADS]; /*each thread gets a queue, last queue is for thread main*/
__thread int id; //TODO delete, dbg threadlocal
int launched_helpers = 0;

/* perfaction function pointers*/
struct traversal * perf_funs;

int transform_is_parallel(void)
{
   return is_parallel;
}

typedef struct
{
   void *addr;
   void *closure_with_args;
   UT_hash_handle hh;
} freelater_entry;
/*thread local storage, each thread has its own free hash*/
__thread freelater_entry *freelater_hash = NULL; 


/* The delayed free code was for the DSU part of the experiements....*/

/* Each thread adds entry to its own thread-local table.  Assuming malloc works
 * as expected (unique address), and because already verifyed that we check
 * below for unique mapping (only free old mem once), we don't need to check
 * first to see if it exists.*/
void transform_delayedfree(void *addr, void * closure_with_args)
{
   freelater_entry *fl_item;
   fl_item = malloc(sizeof(freelater_entry));
   fl_item->addr = addr;
   fl_item->closure_with_args = closure_with_args;
// printf("thread %lu requesting freeing at %p\n", (unsigned long)pthread_self(), addr);
   HASH_ADD_PTR(freelater_hash, addr, fl_item);
}


/*  All closures and all "old mem" is stored in each thread's local storage to
 *  facilitate distributed delete.  When *ALL* threads have finished, this will
 *  delete all old-memory and all closures processed by this thread. */
static void transform_perform_delayedfree(void)
{
   printf("iterating %d items\n", HASH_COUNT(freelater_hash));
   freelater_entry *entry, *tmp;
   HASH_ITER(hh, freelater_hash, entry, tmp)
   {
      HASH_DEL(freelater_hash, entry);
      if(entry->closure_with_args)
      {
         free(entry->closure_with_args);
         free(entry->addr);
      }
      else if(entry->addr)
      {
         printf("freeing at %p\n", entry->addr);
         free(entry->addr);
      }
      free(entry);
   }
}


/* what to do with an int/char/etc */
static void perfaction_prim(void *in, typ type, void *out){
   perf_funs->perfaction_prim(in, type, out);
}

/* return 1 to traverse inside struct. return 0 to not traverse inside struct */
static int perfaction_struct(void *in, typ type, void *out){
   return perf_funs->perfaction_struct(in, type, out);
}

/* return a pointer to continue traversal. return NULL to not traverse this branch further */
static int perfaction_ptr(void **in, typ type, void **out){
   return perf_funs->perfaction_ptr(in, type, out);
}

/* what to do with an already visted heap item */
static void perfaction_ptr_mapped(void **in, typ type, void **out){
   perf_funs->perfaction_ptr_mapped(in, type, out);
}

void init_parallel_tbls(void);
void init_thread_local(void);
void transform_init(struct traversal * funs, int p)
{
   is_parallel = p;
   perf_funs = funs;
   cstrider_type_init();
   populate_visit_all();

   if(transform_is_parallel())
      init_parallel_tbls();
}


typedef void (*fptr_t)(void *in,void *out,typ t);


task * transform_make_enq(void *from, typ t, void* to, int issplit)
{
   if (!from) return NULL; //don't enqueue a null pointer
   task* new_entry = malloc(sizeof(task));
   new_entry->in = from;
   new_entry->type = t;
   new_entry->out = to;
   new_entry->issplit = issplit;
   return new_entry;
}

int visitcalls = 0; //dbg
void transform_free(void)
{

   /* if parallel, delayed free will be in thread-local. if not, must call*/
   if(!transform_is_parallel())
      transform_perform_delayedfree();

   cstrider_type_free();
   cstrider_visit_free();

   //head is already freed inside macro, now just reset the pointer to NULL
   clear_pxform_mappings_head();

   printf("DBG visitcalls = %d\n", visitcalls);
   visitcalls = 0;
}

void callxformer(task *t);
/* enqueue wrapper */
int queue_num =0; //TODO dlete.  for debug only
void addtoqueue(task *t)   //TODO delete this whole function and suck it in to another one...
{

   /*make sure there is something at this address */
   if(!t) return;

///////////TODO MAKE SURE THE NEXT 2 LINES ARE UNCOMMENTED WHEN NOT TESTING
   // callxformer(t);
   // return;


   if(transform_is_parallel())
      enqueue(queue_array, NUM_THREADS, t);
   else
      enqueue(queue_array, 1, t);

}

/* Called from processqueue.  Actually does the work of the dequeued task.
   Calls the transformer function provided with the corresponding (in-out) addrs.
   If it is a closure, it looks up the corresponding addresses and calls the closure. */
void callxformer(task * t)
{
   fptr_t f = cstrider_get_tvers_funptr(t->type);
   if(t->issplit == 1)
      transform_array(t->in, t->type, t->out); //array paralleization
   else
      f(t->in, t->out, t->type);
}


void * processqueue(void * arg)
{

   unsigned long home_queue = (unsigned long)arg; /*queue array index */
   id = home_queue;
   if(home_queue != NUM_THREADS-1) /*each helper has its own deletion table, must init.*/
      init_thread_local();
   start_dequeue(&queue_array[home_queue]); /*thread ready*/
   id = home_queue;

   if(transform_is_parallel())
   {
      //printf("Parallel traversal beginning dequeue for thread %d\n", (int)home_queue);
      task * dqed = dequeue(queue_array, NUM_THREADS, home_queue);
      while(dqed || !allqempty(queue_array, NUM_THREADS))
      {
         if(dqed)
         {
            callxformer(dqed);
            deq_task_finished(&queue_array[home_queue]);
            free(dqed);
         }
         //printf("Parallel traversal calling dequeue for thread %d\n", (int)home_queue);
         dqed = dequeue(queue_array, NUM_THREADS, home_queue);
      }
   }
   else
   {
      task * dqed = dequeue(queue_array, 1, 0);
      while(dqed)
      {
         if(dqed)
         {
            callxformer(dqed);
            deq_task_finished(&queue_array[0]);
            free(dqed);
         }
         dqed = dequeue(queue_array, 1, 0);
      }
   }
   /* thread local delete.*/
   transform_perform_delayedfree();

   /* "When traversal is complete, each thread frees an equal share of the
    * shared traversal-related hash map."  */
   pxform_mapping_entry *entry, *tmp;
   pxform_mapping_entry * head = get_pxform_mappings_head();
   if(transform_is_parallel()) /* distributed delete */
      FREE_ALL_MALLOCED_ENTRIES(hh, head, entry, tmp, home_queue, NUM_THREADS);
   else if (!transform_is_parallel() && head) /* only one thread */
      FREE_ALL_MALLOCED_ENTRIES(hh, head, entry, tmp, 0, 1);

   return NULL;
}

/*TODO move this?*/
void launch_helpers()
{

//TODO, better way to prevent double-call...
   if(launched_helpers == 1)
      return;
   launched_helpers = 1;

   if (transform_is_parallel())
   {
      unsigned long i;
      /* main thread - init q only. init all queues before launching any threads. */
      for (i =0; i<(NUM_THREADS); i++)
      {
         init_queue(&queue_array[i]);
      }
      for (i =0; i<(NUM_THREADS-1); i++)
      {
         pthread_create(&(tid[i]), NULL, &processqueue, (void*)i);
      }
   }
   else
      init_queue(&queue_array[0]);
}

/* do_automigrate is done...no more root nodes. Now wait for traversal to finish
   for each thread */
/*TODO move/rename this?*/
void kitsune_helpers_wait()
{

   if (transform_is_parallel())
   {
      printf("do_automigrate done, now working on queue %d\n", NUM_THREADS-1);
      processqueue((void*)NUM_THREADS-1);
      int i;
      for (i =0; i<(NUM_THREADS-1); i++)
      {
         pthread_join(tid[i], NULL);
         printf(">>>Thread #%d Joined\n", i);
      }
   }
   else
      processqueue(0);

   launched_helpers = 0;
   printf(">>>Done. Main thread (#%d) returning\n", NUM_THREADS-1);
}

/* To avoid thread race to init tbl in XHASH_ADD, init ahead of time.
 *  TODO
 * maybe there is a better solution. UTHASH initializes during the first add.
 * but if 2 threads call this at once:bad.  The problem is that the table is
 * null, so blocking/checking on a null parameter wont change even after the
 * other thread inits.
 *
 * Another benefit of having a "seed" to start the table is that the table will
 * never be accidentally deleted. (When the last entry is deleted, the table is
 * destroyed and would be recreated if another entry was added. This way we are sure
 * that table destruction won't happen until the end.
 */
void init_parallel_tbls(void)
{
   add_mapping((void*)0xFFFFFFFF,(void*)0xFFFFFFFF);/*init 3rd party hashmap with a non-ptr val*/
   freelater_entry *fl_item = malloc(sizeof(freelater_entry));
   fl_item->addr = NULL; //TODO need better placeholder for first entry
   fl_item->closure_with_args = 0;
   HASH_ADD_PTR(freelater_hash, addr, fl_item);
}
/* each helper thread must call this before beginning work*/
void init_thread_local()
{
   freelater_entry *fl_item = malloc(sizeof(freelater_entry));
   fl_item->addr = NULL;
   fl_item->closure_with_args = 0;
   HASH_ADD_PTR(freelater_hash, addr, fl_item);
}



/* These are the same information as the regular cstrider, only memoize first HT lookup.
   This saves about 100ms per 3 million lookups. */
int memoized_is_array(type_hash_entry *te)
{
   return ((te->arrlen) != 0);
}
typ memoized_get_ptrtype(type_hash_entry *te)
{
   return te->target_type;
}
int memoized_is_ptr(type_hash_entry *te)
{
   if(((te->target_type != -1|| (te->type == TYPE_FUNPTR)) || (te->corresp_func_ptr == transform_fptr)) && (te->arrlen == 0))
      return 1;
   return 0;
}
void * memoized_get_tvers_funptr(type_hash_entry *te)
{
   return te->corresp_func_ptr;
}
int memoized_is_funptr(type_hash_entry *te)
{
   if((te->type==16) || (te->corresp_func_ptr == transform_fptr)) return 1;
   else return 0;
}

/* Loops through necessary number of derefs (XF_PTR),
 * if necessary, creates a queue entry (task), else return NULL.
 *
 * Return 0 if there is no reason to addtoqueue, otherwise return entry to addtoqueue */
void visit(void *in, typ type, void *out)
{

   if(type == -1){
      printf("Warning, found type = -1. (extenal library root, or FILE, or mutex, etc)\n");
      return;
   }
   visitcalls++; //bookkeeping for benchmarking, can delete.

   type_hash_entry *t = get_typeinfo_from_table(type); //memoize to save on ht lookups

   /* single char, opaques (ints, opqaue structs, etc).  NOT called for array
    * elements..these are processed in transform_array. */
   if(cstrider_is_prim(type))
   {
      perfaction_prim(in,type, out);
   }
   else if(memoized_is_ptr(t))
   {

      /* make sure there is something to process.... */
      if(!in) return;

      void * lookup;

      /* Store this.  This is because, if in = out, and out if gets malloced to,
        * in is lost.  */
      void *  in_elem = *(void **)in;
      transform_map_wrlock(in_elem);
      if ((lookup = find_mapping(*(void**)in)))
      {
         perfaction_ptr_mapped(in, type, &lookup);
         *(void**)out = lookup;
         transform_map_unlock(in_elem);
      }
      else
      {
         int retc = perfaction_ptr(in, type, out);
         add_mapping(*(void**)in, *(void**)out);
         transform_map_unlock(in_elem);
         if(!retc || memoized_is_funptr(t)) return;
         type = memoized_get_ptrtype(t);
         visit(*(void**)in, type, *(void**)out);
      }
   }
   /* if it's an array, split it up and enq separately. */
   else if(memoized_is_array(t))
   {
      transform_split_array(in, type, out);
   }
   /*   Process the top-level struct, and continue if non-zero return.  */
   else if (perfaction_struct(in, type, out))
      /* if we've made it here, enqueue this individual item*/
      addtoqueue(transform_make_enq(in, type, out, 0));
}




/* split array into the number of threads. not necessarily going on separate queues,
   but will go on the lowest queue....could do better...but breaks up array a bit*/
void transform_split_array(void *in, typ t, void *out)
{

/////////// MAKE SURE THE NEXT 3 LINES ARE UNCOMMENTED WHEN NOT TESTING
   if(!transform_is_parallel())
   {
      transform_array(in, t, out);
      return;
   }

   size_t count = (size_t)cstrider_get_num_array_elems(t);
   if(count < SPLIT)
   {
      transform_array(in, t, out);
      return;
   }

   size_t sz_in = (size_t)cstrider_get_size(t);
   size_t sz_out = (size_t)cstrider_get_out_size(t);

   int i;
   int share = count/SPLIT;
   int remainder = count;
   typ basetype = cstrider_get_array_base_type(t);
   typ splittype = mktyparr(share, basetype);

   for(i=0; i<SPLIT; i++)
   {
      if(i < (SPLIT -1))
      {
         addtoqueue(transform_make_enq(in+(sz_in*i*share), splittype, out+(sz_out*i*share), 1));
         remainder = remainder -share;
      }
      else
      {
         transform_array(in+(sz_in*i*share), mktyparr(remainder, basetype), out+(sz_out*i*share));
      }
   }
}


void transform_array(void *in, typ t, void *out)
{

   size_t count = (size_t)cstrider_get_num_array_elems(t);
   size_t sz_in = (size_t)cstrider_get_size(t);
   size_t sz_out = (size_t)cstrider_get_out_size(t);
   typ basetype = cstrider_get_array_base_type(t);

   char *in_ptr = (char *)in;
   char *out_ptr = (char *)out;

   int i;
   for(i=0; i<count; i++, in_ptr += sz_in, out_ptr += sz_out)
   {
      visit(in_ptr, basetype, out_ptr);
   }
}


//These are wrappers for functions that do not have have generated traversal
//code (i.e. non-structures)
void transform_prim(void *in, void *out, typ t)
{
   visit(in, t, out);
}

void transform_fptr(void *in, void *out, typ t)
{
   visit(in, t, out);

}

