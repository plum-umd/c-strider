#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include "queue.h"


void init_queue(queue *q)
{
   q->front = 0;
   q->rear = QUEUESIZE-1;
   q->thread_started = 0;
   q->count = 0;
   q->inflight = 0;
   pthread_mutex_init(&q->queue_mutex, NULL);
}

/* call when thread is launched and is ready to begin reading from queue.
 * prevents premature exit of threads in case of false empty */
void start_dequeue(queue *q)
{
   pthread_mutex_lock(&(q->queue_mutex));
   q->thread_started = 1;
   pthread_mutex_unlock(&(q->queue_mutex));
}

/* Iterate over ALL the queues
   1 - all empty.
   0 - elements remain. */
/* TODO implement non-blocking/CAS. This is overly complicated. */
int allqdone = 0;
int allqempty(queue queue_array[], int num_queues)
{
   int is_empty = 1;
   int i;

   /* retrieve previous empty if available*/
   if(allqdone) return allqdone;

   /* have all been started? (queues will be empty on startup)*/
   /* is any task in flight? (rough estimate)*/
   for (i =0; i<num_queues; i++)
   {
      if ( !(queue_array[i].thread_started ==1)|| (queue_array[i].inflight >0))
         return 0;
   }
   /* get all locks */
   for (i =0; i<num_queues; i++)
   {
      pthread_mutex_lock(&(queue_array[i].queue_mutex));
   }
   /* check all counts */
   for (i =0; i<num_queues; i++)
   {
      if (!(queue_array[i].inflight == 0) || !(queue_array[i].count ==0) || !(queue_array[i].thread_started ==1))
      {
         is_empty = 0;
         break;
      }
   }
   /* store the result for the next threads. note we have all locks here.*/
   if(is_empty)
      allqdone = 1;

   /* release all locks */
   for (i =0; i<num_queues; i++)
   {
      pthread_mutex_unlock(&(queue_array[i].queue_mutex));
   }

   return is_empty;
}


/* APPROXIMATE (lock-free) count */
int lowest(queue queue_array[], int num_queues)
{
   int lowcount = QUEUESIZE;
   int i, lowqueue = 0;
   for(i = 0; i <num_queues; i++)
   {
      if(queue_array[i].count < lowcount)
      {
         lowqueue = i;
         lowcount = queue_array[i].count;
      }
   }
   return lowqueue;
}

/* APPROXIMATE (lock-free) count */
int highest(queue queue_array[], int num_queues)
{
   int highcount = 0;
   int i, highqueue = 0;
   for(i = 0; i <num_queues; i++)
   {
      if(queue_array[i].count > highcount)
      {
         highqueue = i;
         highcount = queue_array[i].count;
      }
   }
   return highqueue;
}

/* enqueue to the queue with the least number of entries.  if that queue is
 * busy, don't wait, grab another queue until one is free.*/
int enqueue(queue queue_array[], int num_queues, void * x)
{

   int qnum = lowest(queue_array, num_queues);
   int err = pthread_mutex_trylock(&queue_array[qnum].queue_mutex);
   while(err == EBUSY)
   {
      qnum = (qnum+1)%num_queues;
      err = pthread_mutex_trylock(&queue_array[qnum].queue_mutex);
   }
   queue * q = &queue_array[qnum];

   if (q->count >= QUEUESIZE)
   {
      printf("ERROR: queue overflow enqueue x=%p\n",x);
      assert(0);
      pthread_mutex_unlock(&(q->queue_mutex));
      return 0;
   }
   else
   {
      q->rear = (q->rear+1) % QUEUESIZE;
      q->q[ q->rear ] = x;
      q->count = q->count + 1;
      //printf("enqueued %p queue %p\n", x, q);
      pthread_mutex_unlock(&(q->queue_mutex));
      return 1;
   }
}


/* This solves the problem of all queue counts being 0 (seemingly done)...
 * but if there is one task not yet completed (inflight), that task may generate more enqueues.
 * This avoids the "false done" problem. only called by "home queue" for thread*/
void deq_task_finished(queue *q)
{

   pthread_mutex_lock(&(q->queue_mutex));
   q->inflight = q->inflight - 1;
   pthread_mutex_unlock(&(q->queue_mutex));
}


void*  dequeue(queue queue_array[], int num_queues, int homeq)
{
   int qnum = homeq;
   /* try the home queue*/
   int err = pthread_mutex_trylock(&queue_array[qnum].queue_mutex);
   /* if home queue busy, try the high queue*/
   if (err == EBUSY)
   {
      qnum = highest(queue_array, num_queues);
      err = pthread_mutex_trylock(&queue_array[qnum].queue_mutex);
   }
   /* keep trying until free queue....*/
   while(err == EBUSY)
   {
      qnum = (qnum+1)%num_queues;
      err = pthread_mutex_trylock(&queue_array[qnum].queue_mutex);
   }
   queue * q = &queue_array[qnum];
   if(q->count == 0)
   {
      pthread_mutex_unlock(&(q->queue_mutex));
      return NULL;
   }
   void * x = q->q[ q->front ];
   q->front = (q->front+1) % QUEUESIZE;
   q->count = q->count - 1;
   /* track inflight by home queue which receives the "completed" signal.
      Only home q can write to its inflight field*/
   queue_array[homeq].inflight = queue_array[homeq].inflight + 1;
   //printf("dequeued %p queue %p\n", x, q);
   pthread_mutex_unlock(&(q->queue_mutex));
   return(x);
}

//dbg
//void print_queue(queue *q)
//{
//   int j;
//
//   for (j=0; j<QUEUESIZE; j++) {
//        if(q->q[j]!=0)
//           printf("%p ",q->q[j]);
//   }
//
//   printf("| %d ",q->count);
//   printf("\n");
//}


