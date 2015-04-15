/**
    @file thread.h
    @author Vaishali and Ankit
    @brief This is our CSP assignment
*/
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include<stdlib.h>
#include<iostream>

//#define SECOND 1000000
#define STACK_SIZE 4096

void run(int);
void run_ready(int);
void alarm_handler(int);
void dispatch(int);
void resume(int);
typedef void*(*func_args)(void*);

#define NEW 0
#define RUNNING 1
#define READY 2
#define SLEEPING 3
#define SUSPENDED 4
#define COMPLETED 5

#define TIME_QUANTUM 1

//using namespace std;

static int tid , total_threads;
static bool entered_sleep = false , suspend_me = false , came_from_start = false , came_from_join = false ,
            came_from_check = false , in_start = false;
time_t timer , timer_now;
sigjmp_buf env;
int back = 0;

//jmp_buf env;
typedef struct statistics
{
    int thread_id;
    int state;
    int bursts;
    float total_exec_time;
    float total_sleep_time;
    float total_suspend_time;
    float avg_exec_time;
    float avg_wait_time;

}statistics;


typedef struct thread
{
    struct statistics stats;
    void (*func) (void);
    func_args f_args;
    void *args;
    void *ret_value;
    bool arg_present , func_over , joined;
    void (*ret)(void);
    int join_list[10];
    time_t timer_start , timer_now , timer_sleep_start , timer_sleep_now , timer_suspend_start , timer_resume_now ,timer_wait_start,
           timer_wait_now;
    sigjmp_buf env;
    int sleep_time , no_of_joins;
    char stack[STACK_SIZE];

}thread;


thread *current_thread = NULL;


typedef struct Queue
{
  thread t;
  struct Queue *next;
  struct Queue *prev;
} Queue;

Queue *front_R = NULL, *rear_R = NULL , *front_S = NULL , *rear_S = NULL , *front_C = NULL , *rear_C = NULL ,
      *front_Sl = NULL , *rear_Sl = NULL , *front_N = NULL , *rear_N = NULL, *current_node = NULL ;

void insert_readyQueue(thread t)
{
  Queue *node = (Queue*)malloc(sizeof(Queue));;
  node->t = t;
  node->next = NULL;
  node->prev = NULL;

  if(front_R == NULL)
  {
     front_R = node;
     rear_R = node;
  }

  else
  {
     rear_R->next = node;
     node->prev = rear_R;
     rear_R = rear_R->next;
  }

}

void insert_node_readyQueue(Queue *node)
{

  if(front_R == NULL)
  {
     front_R = node;
     rear_R = node;
  }

  else
  {
     rear_R->next = node;
     node->prev = rear_R;
     rear_R = rear_R->next;
  }

}

Queue* delete_readyQueue()
{
  Queue *node;
  if(front_R == NULL)
  {
     printf("\n no more threads to execute ... \n");
     return NULL;
  }

  else if(front_R == rear_R)
  {
     node  = front_R;
     node->next = NULL;
     node->prev = NULL;
     front_R = NULL;
     rear_R  = NULL;
     return node;
  }

  else
  {
     node  = front_R;
     front_R = front_R->next;
     node->next = NULL;
     node->prev = NULL;
     return node;
  }
}

Queue* search_readyQueue(int threadID)
{
  Queue *node;
  node = front_R;
  while( ( node != NULL )  && (node->t.stats.thread_id != threadID) )
      node = node->next;

  return node;  //// check for node == NULL case while calling this function

}

Queue* delete_specific_readyQueue(int threadID)
{
    Queue *node = search_readyQueue(threadID);
    if( node == NULL)
        return NULL;
    if(node == front_R)
    {
       node = delete_readyQueue();
       return node;
    }
    if(node == rear_R)
    {
       rear_R = rear_R->prev;
       rear_R->next = NULL;
       node->next = NULL;
       node->prev = NULL;
       return node;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = NULL;
    node->next = NULL;
    return node;

}


void insert_newQueue(thread t)
{
  Queue *node = (Queue*)malloc(sizeof(Queue));;
  node->t = t;
  node->next = NULL;
  node->prev = NULL;

  if(front_N == NULL)
  {
     front_N = node;
     rear_N = node;
  }

  else
  {
     rear_N->next = node;
     node->prev = rear_N;
     rear_N = rear_N->next;
  }

}

void insert_node_newQueue(Queue *node)
{

  if(front_N == NULL)
  {
     front_N = node;
     rear_N = node;
  }

  else
  {
     rear_N->next = node;
     node->prev = rear_N;
     rear_N = rear_N->next;
  }

}

Queue* delete_newQueue()
{
  Queue *node;
  if(front_N == NULL)
  {
     return NULL;
  }

  else if(front_N == rear_N)
  {
     node  = front_N;
     node->next = NULL;
     node->prev = NULL;
     front_N = NULL;
     rear_N  = NULL;
     return node;
  }

  else
  {
     node  = front_N;
     front_N = front_N->next;
     node->next = NULL;
     node->prev = NULL;
     return node;
  }
}

Queue* search_newQueue(int threadID)
{
  Queue *node;
  node = front_N;
  while( ( node != NULL )  && (node->t.stats.thread_id != threadID) )
      node = node->next;

  return node;  //// check for node == NULL case while calling this function

}

Queue* delete_specific_newQueue(int threadID)
{
    Queue *node = search_newQueue(threadID);
    if( node == NULL)
        return NULL;
    if(node == front_N)
    {
       node = delete_newQueue();
       return node;
    }
    if(node == rear_N)
    {
       rear_N = rear_N->prev;
       rear_N->next = NULL;
       node->next = NULL;
       node->prev = NULL;
       return node;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = NULL;
    node->next = NULL;
    return node;

}

void insert_suspendedQueue(thread t)
{
  Queue *node = (Queue*)malloc(sizeof(Queue));;
  node->t = t;
  node->next = NULL;
  node->prev = NULL;

  if(front_S == NULL)
  {
     front_S = node;
     rear_S = node;
  }

  else
  {
     rear_S->next = node;
     node->prev = rear_S;
     rear_S = rear_S->next;
  }

}

void insert_node_suspendedQueue(Queue *node)
{

  if(front_S == NULL)
  {
     front_S = node;
     rear_S = node;
  }

  else
  {
     rear_S->next = node;
     node->prev = rear_S;
     rear_S = rear_S->next;
  }


}

Queue* delete_suspendedQueue()
{
  Queue *node;
  if(front_S == NULL)
  {
     printf("\n no more threads to delete/resume ... \n");
     return NULL;
  }

  else if(front_S == rear_S)
  {
     node  = front_S;
     node->next = NULL;
     node->prev = NULL;
     front_S = NULL;
     rear_S  = NULL;
     return node;
  }

  else
  {
     node  = front_S;
     front_S = front_S->next;
     node->next = NULL;
     node->prev = NULL;
     return node;
  }
}


Queue* search_suspendedQueue(int threadID)
{
  Queue *node;
  node = front_S;
  while( ( node != NULL )  && (node->t.stats.thread_id != threadID) )
      node = node->next;

  return node;  //// check for node == NULL case while calling this function

}

Queue* delete_specific_suspendedQueue(int threadID)
{
    Queue *node = search_suspendedQueue(threadID);

    if( node == NULL)
        return NULL;
    if(node == front_S)
    {
       node = delete_suspendedQueue();
       return node;
    }
    if(node == rear_S)
    {
       rear_S = rear_S->prev;
       rear_S->next = NULL;
       node->next = NULL;
       node->prev = NULL;
       return node;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = NULL;
    node->next = NULL;
    return node;

}


void insert_sleepingQueue(thread t)
{
  Queue *node = (Queue*)malloc(sizeof(Queue));;
  node->t = t;
  node->next = NULL;
  node->prev = NULL;

  if(front_Sl == NULL)
  {
     front_Sl = node;
     rear_Sl = node;
  }

  else
  {
     rear_Sl->next = node;
     node->prev = rear_Sl;
     rear_Sl = rear_Sl->next;
  }

}


void insert_node_sleepingQueue(Queue *node)
{

  if(front_Sl == NULL)
  {
     front_Sl = node;
     rear_Sl = node;
  }

  else
  {
     rear_Sl->next = node;
     node->prev = rear_Sl;
     rear_Sl = rear_Sl->next;
  }


}


Queue* delete_sleepingQueue()
{
  Queue *node;
  if(front_Sl == NULL)
  {
     printf("\n no more threads to delete/wake up ... \n");
     return NULL;
  }

  else if(front_Sl == rear_Sl)
  {
     node  = front_Sl;
     node->next = NULL;
     node->prev = NULL;
     front_Sl = NULL;
     rear_Sl  = NULL;
     return node;
  }

  else
  {
     node  = front_Sl;
     front_Sl = front_Sl->next;
     node->next = NULL;
     node->prev = NULL;
     return node;
  }
}


Queue* search_sleepingQueue(int threadID)
{
  Queue *node;
  node = front_Sl;
  while( ( node != NULL )  && (node->t.stats.thread_id != threadID) )
      node = node->next;

  return node;  //// check for node == NULL case while calling this function

}


Queue* delete_specific_sleepingQueue(int threadID)
{
    Queue *node = search_sleepingQueue(threadID);

    if( node == NULL)
        return NULL;
    if(node == front_Sl)
    {
       node = delete_sleepingQueue();
       return node;
    }
    if(node == rear_Sl)
    {
       rear_Sl = rear_Sl->prev;
       rear_Sl->next = NULL;
       node->next = NULL;
       node->prev = NULL;
       return node;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = NULL;
    node->next = NULL;
    return node;

}



void insert_completedQueue(thread t)
{
  Queue *node = (Queue*)malloc(sizeof(Queue));;
  node->t = t;
  node->next = NULL;
  node->prev = NULL;

  if(front_C == NULL)
  {
     front_C = node;
     rear_C = node;
  }

  else
  {
     rear_C->next = node;
     node->prev = rear_C;
     rear_C = rear_C->next;
  }

}


void insert_node_completedQueue(Queue *node)
{

  if(front_C == NULL)
  {
     front_C = node;
     rear_C = node;
  }

  else
  {
     rear_C->next = node;
     node->prev = rear_C;
     rear_C = rear_C->next;
  }

}

Queue* delete_completedQueue()
{
  Queue *node;
  if(front_C == NULL)
  {
     //printf("\n no more threads to execute ... \n");
     return NULL;
  }

  else if(front_C == rear_C)
  {
     node  = front_C;
     node->next = NULL;
     node->prev = NULL;
     front_C = NULL;
     rear_C  = NULL;
     return node;
  }

  else
  {
     node  = front_C;
     front_C = front_C->next;
     node->next = NULL;
     node->prev = NULL;
     return node;
  }
}


Queue* search_completedQueue(int threadID)
{
  Queue *node;
  node = front_C;
  while( ( node != NULL )  && (node->t.stats.thread_id != threadID) )
      node = node->next;

  return node;  //// check for node == NULL case while calling this function

}


Queue* delete_specific_completedQueue(int threadID)
{
    Queue *node = search_readyQueue(threadID);
    if( node == NULL)
        return NULL;
    if(node == front_C)
    {
       node = delete_readyQueue();
       return node;
    }
    if(node == rear_C)
    {
       rear_C = rear_C->prev;
       rear_C->next = NULL;
       node->next = NULL;
       node->prev = NULL;
       return node;
    }
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = NULL;
    node->next = NULL;
    return node;

}


#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/** @brief In x86_64 architecture, FS:[0x30] refers to the 4 bytes starting from [0x30] in the segment register FS.
  * These 4 bytes store the linear address of PROCESS ENVIRONMENT BLOCK - PEB.
  * @param addr : Address of type long to be translated
  * @return Translated address
  */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
        "rol    $0x11,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/** @brief In i386 architecture, GS:[0x18] refers to the 4 bytes starting from [0x18] in the segment register GS.
  * These 4 bytes store the linear address of PROCESS ENVIRONMENT BLOCK - PEB.
  * @param addr : Address of type long to be translated
  * @return Translated address
  */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
        "rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif

/** @brief Create a thread with single argument
 *
 *  f is a ptr to a function which
 * takes (void *) and returns (void *). Unlike the threads so far, this thread takes
 * arguments, and instead of uselessly looping forever, returns a value in a (void *).
 * This function returns the id of the thread created.
 *
 *  @param f f is a ptr to a function which takes (void *) and returns (void *).
 *  @param args Argument taken by the function to which f is pointing
 *  @return This function returns the id of the thread created.
 */
int createWithArgs(void* (*f)(void *) , void *args)
{
   Queue *node = (Queue*)malloc(sizeof(Queue));
   node->t.stats.thread_id = tid;
   node->t.stats.state = NEW;
   node->t.f_args = f;
   node->t.args = args;
   node->t.arg_present = true;
   node->t.func_over = false;
   node->t.joined = false;
   node->t.ret_value = NULL;
   node->t.no_of_joins = 0;
   node->t.stats.bursts = 0;
   node->t.stats.total_exec_time = 0;
   node->t.stats.avg_exec_time = 0;
   node->t.stats.total_sleep_time = 0;
   node->t.stats.avg_wait_time = 0;
   node->t.stats.total_suspend_time = 0;
   for(int i =0;i<10;i++)
      node->t.join_list[i]=-1;
   node->prev = NULL;
   node->next = NULL;
   insert_node_newQueue(node);
   tid++;
   total_threads++;
   return tid-1;
}

/** @brief Creates a thread
 *
 *  creates a thread, whose execution starts from
 *  function f which has no parameter and no return type; however create returns
 *  thread ID of created thread.
 *
 *  @param f pointer to a function which has no parameter and void return type
 *  @return id of the thread created
 */
int create(void(*f)(void))
{
   Queue *node = (Queue*)malloc(sizeof(Queue));
   node->t.stats.thread_id = tid;
   node->t.stats.state = NEW;
   node->t.func = f;
   node->t.arg_present = false;
   node->t.func_over = false;
   node->t.joined = false;
   node->t.ret_value = NULL;
   node->t.no_of_joins = 0;
   node->t.stats.bursts = 0;
   node->t.stats.total_exec_time = 0;
   node->t.stats.avg_exec_time = 0;
   node->t.stats.total_sleep_time = 0;
   node->t.stats.avg_wait_time = 0;
   node->t.stats.total_suspend_time = 0;
   for(int i =0;i<10;i++)
      node->t.join_list[i]=-1;
   node->prev = NULL;
   node->next = NULL;
   insert_node_newQueue(node);
   tid++;
   total_threads++;
   return tid-1;
}

void return_address(void)
{
   //Queue *node = search_readyQueue(threadID);
   if(current_node->t.arg_present)
       current_node->t.ret_value = current_node->t.f_args(current_node->t.args);
   else
        current_node->t.func();
   current_node->t.func_over = true;
   current_node->t.stats.state = COMPLETED;
   if(current_node->t.joined)
   {
      for(int i=0;i<current_node->t.no_of_joins ;i++)
          resume(current_node->t.join_list[i]);
   }
   dispatch(1);
}

void * return_value(int threadID)
{
   Queue *node = search_completedQueue(threadID);
   if(node != NULL)
   {
      if(node->t.arg_present)
            return node->t.ret_value;

      else
      {
            printf("\n function associated with thread of thread ID %d does not return any value \n",threadID);
            return NULL;
      }
   }

   else
   {
       printf("\n function execution associated with thread of thread ID %d still not over\n",threadID);
       return NULL;
   }

}


/** @brief Scheduler for our thread library
 *  @param sig signal generated by alarm handler which calls dispatcher
 */
void dispatch(int sig)
{
  signal(SIGALRM,SIG_IGN);
  int ret_val;
  if(!came_from_check)
    ret_val = sigsetjmp(current_node->t.env,1);
  Queue * node , *sleep_node = front_Sl;// = current_node;
  int thread_id , i=0;

  time(&current_node->t.timer_now);
  current_node->t.stats.total_exec_time += difftime(current_node->t.timer_now , current_node->t.timer_start);

  while(sleep_node != NULL)
  {
     time(&sleep_node->t.timer_sleep_now);
     if(difftime(sleep_node->t.timer_sleep_now , sleep_node->t.timer_sleep_start) >= sleep_node->t.sleep_time)
     {
         sleep_node->t.stats.total_sleep_time += sleep_node->t.sleep_time;
         resume(sleep_node->t.stats.thread_id);
     }
     sleep_node = sleep_node->next;
  }


  if (ret_val == 1) {
      return;
  }

  if(current_node == NULL)
  {  printf("i came in dispatch");
      siglongjmp(env,1);

  }
  if(current_node->t.func_over == true )
  {
     if(current_node->next != NULL)
     {
        //printf("\n current function execution over ..\n");
        //printf("switching to other function  \n");
        thread_id = current_node->next->t.stats.thread_id;
        node = delete_readyQueue();
        insert_node_completedQueue(node);
        alarm(TIME_QUANTUM);
        signal(SIGALRM,alarm_handler);
        run_ready(thread_id);
     }
     else
     {
        Queue *node = delete_readyQueue();
        insert_node_completedQueue(node);
        //printf("\n current function execution over ..\n");
        //printf("\n ready queue empty \n");
        current_node = NULL;
        current_thread = NULL;
        siglongjmp(env,1);
     }
  }

  else if(suspend_me)
  {
     suspend_me = false;
     if(current_node->next != NULL)
     {
        thread_id = current_node->next->t.stats.thread_id;
        node = delete_readyQueue();
        insert_node_suspendedQueue(node);
        alarm(TIME_QUANTUM);
        signal(SIGALRM,alarm_handler);
        run_ready(thread_id);
     }
     else
     {
        Queue *node = delete_readyQueue();
        insert_node_suspendedQueue(node);
        printf("\n ready queue empty \n");
        current_node = NULL;
        current_thread = NULL;
        siglongjmp(env,1);
     }
  }

  else if(entered_sleep)
  {
     entered_sleep = false;
     if(current_node->next != NULL)
     {
        thread_id = current_node->next->t.stats.thread_id;
        node = delete_readyQueue();
        insert_node_sleepingQueue(node);
        alarm(TIME_QUANTUM);
        signal(SIGALRM,alarm_handler);
        run_ready(thread_id);
     }
     else
     {
        Queue *node = delete_readyQueue();
        insert_node_sleepingQueue(node);
        //printf("\n ready queue empty \n");
        current_node = NULL;
        current_thread = NULL;
        siglongjmp(env,1);
     }
  }

  else if(current_node->next != NULL)
  {
    thread_id = current_node->next->t.stats.thread_id;
   // printf("switching to other function .. \n");
    time(&current_node->t.timer_wait_start);
    node = delete_readyQueue();
    node->t.stats.state = READY;
    insert_node_readyQueue(node);
    alarm(TIME_QUANTUM);
    signal(SIGALRM,alarm_handler);
    run_ready(thread_id);
  }
  else if(current_node->next == NULL)
  {
    alarm(TIME_QUANTUM);
    signal(SIGALRM,alarm_handler);
    run_ready(current_node->t.stats.thread_id);
  }
  else
  {
     //printf("\n in dispatcher : no more threads to execute \n");
     current_node = NULL;
     current_thread = NULL;
     siglongjmp(env,1);
  }

  //alarm(TIME_QUANTUM);
  //signal(SIGALRM,alarm_handler);
}


void alarm_handler(int sig)
{
  alarm(TIME_QUANTUM);
  signal(SIGALRM, alarm_handler);
  dispatch(1);
}

void setup(int threadID)
{
    address_t sp , pc ;
    Queue *node;
    node = search_newQueue(threadID);
    if(node == NULL)
    {
       printf("\n thread with thread ID %d could not be found \n",threadID);
       return;
    }
    node->t.ret = return_address;
    sp = (address_t)(node->t.stack) + STACK_SIZE - sizeof(address_t);
    pc = (address_t)(node->t.ret);
    sigsetjmp(node->t.env, 1);
    (node->t.env->__jmpbuf)[JB_SP] = translate_address(sp);
    (node->t.env->__jmpbuf)[JB_PC] = translate_address(pc);
    sigemptyset(&(node->t.env->__saved_mask));
}

void check_sleepingQueue()
{
  // signal(SIGALRM,SIG_IGN);
   Queue *sleep_node = front_Sl;
   //printf("\ni have entered check sleeping\n");
   while(sleep_node != NULL)
   {
     time(&sleep_node->t.timer_sleep_now);
     if(difftime(sleep_node->t.timer_sleep_now , sleep_node->t.timer_sleep_start) >= sleep_node->t.sleep_time)
      {
        came_from_check = true;
        resume(sleep_node->t.stats.thread_id);
        back = 2;
        break;
      }
     //printf("\n sleeping..");
     sleep_node = sleep_node->next;
  }

}



/** @brief Called by your main function. One called,starts running of thread
 *  and never return.(so at least one of your thread should have infinite loop.)
 */
void start()
{
  came_from_start = true;
  back = 1;
  for(int i=0;i<total_threads;i++)
    run(i);

  alarm(TIME_QUANTUM);
  signal(SIGALRM, alarm_handler);
   sigsetjmp(env , 1);

  if(!in_start)
  {
    in_start = true;
    run_ready(front_R->t.stats.thread_id);
  }

  while(front_Sl != NULL && back == 1)
  {
     check_sleepingQueue();
     usleep(1000000);
  }
  return;
}


/** @brief Submits particular thread to scheduler for execution.
 *  Could be called from any other thread.
 *  @param threadID Id of thread which is to be run
 */
void run(int threadID)
{
   setup(threadID);
   Queue *node = delete_specific_newQueue(threadID);
   node->t.stats.state = READY;
   time(&node->t.timer_wait_start);
   insert_node_readyQueue(node);
   if(front_R == node && came_from_start == false)
   {
      alarm(TIME_QUANTUM);
      signal(SIGALRM, alarm_handler);
      run_ready(front_R->t.stats.thread_id);
   }
}


void run_ready(int threadID)
{
    Queue *node = search_readyQueue(threadID);
    current_node = node;
    current_thread = &(current_node->t);
    time(&current_node->t.timer_start);
    time(&current_node->t.timer_wait_now);
    current_node->t.stats.bursts++;
    current_node->t.stats.avg_wait_time += difftime(current_node->t.timer_wait_now , current_node->t.timer_wait_start);
    current_thread->stats.state = RUNNING;
    siglongjmp(current_node->t.env, 1);

}


/** @brief Suspends a thread till resume is not called for that
 *  thread.
 *  @param threadID Id of the thread to be suspended
 */
void suspend(int threadID)
{
    Queue *node;
    if(entered_sleep)
       node = search_readyQueue(threadID);

    else
       node = search_readyQueue(threadID);
    if(node == NULL)
    {
        node = search_suspendedQueue(threadID);
        //if(node == NULL)
            //printf("\n there is no such thread with thread ID  %d \n",threadID);

        //else
            //printf("\n thread with thread ID  %d  is already suspended \n",threadID);

        return;

    }

    if(entered_sleep)
    {
       node->t.stats.state = SLEEPING;
       dispatch(1);
       return;
    }
    if(came_from_join)
    {
       came_from_join = false;
    }
    else
       //printf("\n thread with thread id  '%d'  has been suspended \n",node->t.stats.thread_id);
    node->t.stats.state = SUSPENDED;
    time(&node->t.timer_suspend_start);
    if(node == current_node)
    {
       suspend_me = true;
       dispatch(1);
       return;
    }
    node = delete_specific_readyQueue(threadID);
    insert_node_suspendedQueue(node);

}


/** @brief Resumes the particular thread
 *  @param threadID of the thread to be resumed
 */
void resume(int threadID)
{
    Queue *node;
    node = delete_specific_suspendedQueue(threadID);
    if(node == NULL)
    {
        node = search_sleepingQueue(threadID);
        if(node == NULL)
        {
            node = search_readyQueue(threadID);
            if(node == NULL)
                 printf("\n there is no such thread with thread ID  %d \n",threadID);

            else
                 printf("\n thread with thread ID  %d  is already resumed \n",threadID);

            return;
        }

        else
        {
            node = delete_specific_sleepingQueue(threadID);
            node->t.stats.state = READY;
            insert_node_readyQueue(node);
            //printf("\n thread with thread ID %d has exit sleep mode \n",node->t.stats.thread_id);
            if(front_R == node)
            {
                current_node = node;
                //run_ready(current_node->t.stats.thread_id);
                dispatch(1);
            }

            return;

        }

    }
    //printf("\n thread with thread id  '%d'  has been resumed now \n",node->t.stats.thread_id);
    time(&node->t.timer_resume_now);
    node->t.stats.total_suspend_time += difftime(node->t.timer_resume_now , node->t.timer_suspend_start);
    node->t.stats.state = READY;
    insert_node_readyQueue(node);
    if(front_R == node)
    {
         current_node = node;

         dispatch(1);
    }

}

/** @brief calling thread passes control to another next thread.
 */
void yield()
{
   //printf("\n control has been yielded to next thread \n");
   dispatch(1);
}


/** @brief Deletes a particular thread
 *  @param threadID of the thread to be deleted
 */
void deleteThread(int threadID)
{
    Queue *node;
    node = delete_specific_readyQueue(threadID);
    if(node == NULL)
    {
        node = delete_specific_suspendedQueue(threadID);
        if(node == NULL)
        {
            node = delete_specific_sleepingQueue(threadID);
            if(node == NULL)
            {
               node = delete_specific_completedQueue(threadID);
               if(node == NULL)
               {
       //           printf("\n there is no such thread with thread ID  %d \n",threadID);
                  return ;
               }
            }
        }
    }
     //printf("\n thread with thread id  '%d'  has been deleted \n",node->t.stats.thread_id);
     delete(node);

}



/** @brief returns the status of threaded by
 *  returning pointer to its statistics or NULL.
 *  @param threadId of thread whose status is to be retrieved
 *  @return statistics* which is Thread Control Block of the requested thread
 */
statistics* getStatus(int threadID)
{
    statistics *stats = (statistics*)malloc(sizeof(statistics));
    Queue *node;

    node = search_readyQueue(threadID);

    if(node == NULL)
    {
        node = search_suspendedQueue(threadID);
    }

    if(node == NULL)
    {
        node = search_sleepingQueue(threadID);
    }

    if(node == NULL)
    {
        node = search_completedQueue(threadID);
    }

    if(node == NULL)
    {
        //printf("\n there is no such function with thread ID  %d \n",threadID);
        return NULL;
    }

    node->t.stats.avg_exec_time = (node->t.stats.total_exec_time)/(node->t.stats.bursts);
    stats->avg_exec_time = node->t.stats.avg_exec_time;
    node->t.stats.avg_wait_time = (node->t.stats.avg_wait_time - node->t.stats.total_sleep_time - node->t.stats.total_suspend_time)/(node->t.stats.bursts);
    stats->avg_wait_time = node->t.stats.avg_wait_time;
    stats->bursts = node->t.stats.bursts;
    stats->state = node->t.stats.state;
    stats->thread_id = node->t.stats.thread_id;
    stats->total_exec_time = node->t.stats.total_exec_time;
    stats->total_sleep_time = node->t.stats.total_sleep_time;

    return stats;

}


/** @brief Returns the ID of the calling thread
 *  @return ID of the calling thread
 */
int getID()
{
   return (current_node->t.stats.thread_id);
}


int count = 0;


/** @brief Sleeps the calling thread for sec seconds.
 *  @param sec - number of seconds for which the thread will sleep
 */
void sleepThread(int sec)
{
  time_t time_now;
  time(&current_node->t.timer_sleep_start);
  current_node->t.sleep_time = sec;
  //printf("\n thread with thread ID %d has entered sleep mode \n",current_node->t.stats.thread_id);
  entered_sleep = true;
  suspend(current_thread->stats.thread_id);

}


/** @brief Calling thread blocks itself for a thread to
 *  finishes.
 *  @param threadID After this thread finishes execution, caller will resume execution
 */
void join(int threadID)
{
  Queue *node = search_readyQueue(threadID);
  if(node == NULL)
  {
     node = search_suspendedQueue(threadID);
     if(node == NULL)
     {
         node = search_sleepingQueue(threadID);
         if(node == NULL)
         {
             node = search_newQueue(threadID);
             if(node == NULL)
             {
                 node = search_completedQueue(threadID);
                 if(node == NULL)
                 {
    //                printf("\n there is no thread with thread ID %d to join with \n",threadID);
                 }

                 else
                 {
      //              printf("\n thread with thread ID %d has already finished its execution \n",threadID);
                 }

                 return;
             }
         }
     }
  }

  node->t.joined = true;
  node->t.join_list[(node->t.no_of_joins)++] = current_node->t.stats.thread_id;
  came_from_join = true;
  //printf("\n thread with thread id  '%d'  has been joined  with thread of thread ID %d \n",current_node->t.stats.thread_id , threadID);
  suspend(current_node->t.stats.thread_id);

}


/** @brief waits till a thread created
 *  with the above function returns, and returns the return value of that thread. This
 *  function, obviously, waits until that thread is done with.
 *  @param threadID Id of the thread
 */
void *GetThreadResult(int threadID)
{
  join(threadID);
  Queue * node = search_completedQueue(threadID);

  return node->t.ret_value;
}


/** @brief Prints the statistics of the current environment
 *  @param *stats Pointer to the statistics
 */
void print_stats(statistics *stats)
{
   printf("\n thread ID  :  %d",stats->thread_id);
      switch(stats->state)
      {
         case 0 : printf("\n thread State  :  NEW ");
                  break;

         case 1 : printf("\n thread State  :  RUNNING ");
                  break;

         case 2 : printf("\n thread State  :  READY ");
                  break;

         case 3 : printf("\n thread State  :  SLEEPING ");
                  break;

         case 4 : printf("\n thread State  :  SUSPENDED ");
                  break;

         case 5 : printf("\n thread State  :  COMPLETED ");
                  break;

        default : printf("\n thread State  is INVALID ");

      }

      printf("\n No. of Bursts  :  %d",stats->bursts);
      printf("\n Total execution time (in msec)  :  %f",(stats->total_exec_time)*1000);
      printf("\n Total requested sleeping time (in msec)  :  %f",(stats->total_sleep_time)*1000);
      printf("\n Average execution time quantum (in msec)  :  %f",(stats->avg_exec_time)*1000);
      printf("\n Average waiting time (status = READY) (in msec)  :  %f \n",(stats->avg_wait_time)*1000);

}



/** @brief stop scheduler, frees all the space allocated, print statistics per
 *  thread.
 */
void clean()
{
  signal(SIGALRM , SIG_IGN);

  Queue *node = front_R;

  statistics *stats = NULL;
  current_node = NULL;
  current_thread = NULL;
  printf("\n printing the statistics per thread : \n\n");
  while(node != NULL)
  {
      //stats = getStatus(node->t.stats.thread_id);
      node->t.stats.avg_exec_time = (node->t.stats.total_exec_time)/(node->t.stats.bursts);
      node->t.stats.avg_wait_time = (node->t.stats.avg_wait_time - node->t.stats.total_sleep_time - node->t.stats.total_suspend_time)/(node->t.stats.bursts);
      print_stats(&node->t.stats);
      node = delete_readyQueue();
      delete(node);
      node = front_R;
  }

  node = front_N;
  while(node != NULL)
  {
      //stats = getStatus(node->t.stats.thread_id);
      node->t.stats.avg_exec_time = (node->t.stats.total_exec_time)/(node->t.stats.bursts);
      node->t.stats.avg_wait_time = (node->t.stats.avg_wait_time - node->t.stats.total_sleep_time - node->t.stats.total_suspend_time)/(node->t.stats.bursts);
      print_stats(&node->t.stats);
      node = delete_newQueue();
      delete(node);
      node = front_N;
  }

  node = front_S;
  while(node != NULL)
  {
      //stats = getStatus(node->t.stats.thread_id);
      node->t.stats.avg_exec_time = (node->t.stats.total_exec_time)/(node->t.stats.bursts);
      node->t.stats.avg_wait_time = (node->t.stats.avg_wait_time - node->t.stats.total_sleep_time - node->t.stats.total_suspend_time)/(node->t.stats.bursts);
       print_stats(&node->t.stats);
       node = delete_suspendedQueue();
       delete(node);
       node = front_S;
  }

  node = front_C;
  while(node != NULL)
  {
      //stats = getStatus(node->t.stats.thread_id);
      node->t.stats.avg_exec_time = (node->t.stats.total_exec_time)/(node->t.stats.bursts);
      node->t.stats.avg_wait_time = (node->t.stats.avg_wait_time - node->t.stats.total_sleep_time - node->t.stats.total_suspend_time)/(node->t.stats.bursts);
      print_stats(&node->t.stats);
      node = delete_completedQueue();
      delete(node);
      node = front_C;
  }

  node = front_Sl;
  while(node != NULL)
  {
      //stats = getStatus(node->t.stats.thread_id);
      node->t.stats.avg_exec_time = (node->t.stats.total_exec_time)/(node->t.stats.bursts);
      node->t.stats.avg_wait_time = (node->t.stats.avg_wait_time - node->t.stats.total_sleep_time - node->t.stats.total_suspend_time)/(node->t.stats.bursts);
      print_stats(&node->t.stats);
      node = delete_sleepingQueue();
      delete(node);
      node = front_Sl;
  }
   exit(0);
  //signal(SIGALRM,alarm_handler);
  //dispatch(1);

}
/*

thread id
2. state (running, ready, sleeping, suspended).
3. number of bursts (none if the thread never ran).
4. total execution time in msec(N/A if thread never ran).
5. total requested sleeping time in msec (N/A if thread never slept).
6. average execution time quantum in msec (N/A if thread never ran).
7. average waiting time (status = READY) (N/A if thread never ran).


#define NEW 0
#define RUNNING 1
#define READY 2
#define SLEEPING 3
#define SUSPENDED 4
#define COMPLETED 5


int thread_id;
    int state;
    int bursts;
    int total_exec_time;
    int total_sleep_time;
    int avg_exec_time;
    int avg_wait_time;
*/
