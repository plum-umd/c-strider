#ifndef TRANSFORM_INTERNAL_H_
#define TRANSFORM_INTERNAL_H_
typedef long typ;

void transform_init(int parallel);
void transform_free(void);
void launch_helpers(void);
void kitsune_helpers_wait(void);
void kitsune_delayedfree(void *addr, void * closure_with_args);
void visit(void *in, typ type, void *out); 
int transform_is_parallel(void);

void transform_prim(void *in, void *out, typ gen);
typedef struct _task
{
   void* in;
   typ type;
   void* out;
   int issplit;
}task;
void transform_prim(void *in, void *out, typ gen); //TODO deprecated
void transform_fptr(void *in, void *out, typ gen); //TODO deprecated
void transform_split_array(void *in, typ t, void *out);
void transform_array(void *in, typ t, void *out);
void transform_delayedfree(void *addr, void * closure_args);
task * transform_make_enq(void *from, typ t, void* to, int issplit);
void addtoqueue(task *t);  //TODO delete this whole function and suck it in to another one...
int visit_ptr(void *in, typ type, void *out); 
void init_parallel_tbls(void);

#endif
