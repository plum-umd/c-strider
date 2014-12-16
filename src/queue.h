#ifndef queue_h
#define queue_h

#define QUEUESIZE       250000//TODO this will assert(0) on full...

typedef struct {
  void * q[QUEUESIZE];       /* content */
  pthread_mutex_t queue_mutex; /* locking*/
  int front;              /* positioning */       
  int rear;                      
  int thread_started;
  int count;  		/* number of items in queue*/
  int inflight;         /* number of in-flight tasks...may generate more enqueues*/
} queue;


int allqempty(queue queue_array[], int num_queues);
void start_dequeue(queue *q);
void deq_task_finished(queue *q);
void init_queue(queue *q);
int enqueue(queue queue_array[], int num_queues, void * x);
void*  dequeue(queue queue_array[], int num_queues, int homeq);


#endif
