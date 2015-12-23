#include "ds/ds_list.h"
#include "ds/ds_hash.h"
#include "ds/ds_queue.h"
//Salman
#include<map>
#define NA -9999
#define TSK_MAX 1024

#define UNLOCK                  0
#define LOCK                    1
#define FORK                    2
#define THREADINIT              3
#define THREADEXIT              4
#define JOIN                    5
#define LOOP                    6
#define ATTACHQ                 7
#define ENABLELIFECYCLE         8
#define ENABLEEVENT             9
#define POST                    10
#define CALLMETHOD                    11
#define RET                     12
#define READ                    13
#define READSTATIC              14
#define WRITE                   15
#define WRITESTATIC             16

//#define USE_EVENT_ENABLE
#define USE_POST_FRONT
//#define USE_POST_EXTERNAL

//#define USE_FASTTRACK_ALG

typedef int thread_id_t;
typedef int tsk_id_t;
typedef int event_id_t;
typedef int address_t;
typedef int clck_t;

////////////////////////////////////////////////////////////////////////////////////////////////
//

typedef struct _thread_state_t {
//thread_id_t tid;
  clck_t fork_vc[TSK_MAX];     // vc at fork (parent)
  clck_t attachq_vc[TSK_MAX];  // vc at attach-q
  ds_queue *prev_tsk_q;     // task queue
  tsk_id_t cur_tsk_id;      // current tsk id
#ifdef USE_POST_EXTERNAL
  int external; 
#endif
} thread_state_t;

// NOTE: tsk_id == msg_id

typedef struct _task_state_t {
//tsk_id_t tsk_id;         // == msg_id
  clck_t begin_vc[TSK_MAX];    // vc at begin
  clck_t vc[TSK_MAX];          // current vc
  thread_id_t tid;          // current thread id
  tsk_id_t parent_tsk_id;
} task_state_t;

typedef struct _msg_state_t {
//tsk_id_t tsk_id;         // == msg_id, target tsk_id
  thread_id_t src_tid;      // source thread id
  clck_t post_vc[TSK_MAX];     // vc at post
  int delay;                // the amount of delay
#ifdef USE_POST_FRONT
  int front;                // true if SendAtFront
#endif
#ifdef USE_POST_EXTERNAL
  int external;             // true if comes from the external world (system)
#endif
} msg_state_t;

typedef struct _event_state_t {
//event_id_t event_id;
  clck_t enable_vc[TSK_MAX];   // vc at enable
} event_state_t;

typedef struct _var_state_t {
  clck_t W;                    // clock at the last write
  clck_t R;                    // clock at the last read (exclusive)
  clck_t Rvc[TSK_MAX];         // used iff R == READ_SHARED
  tsk_id_t W_tsk_id;
  tsk_id_t R_tsk_id;
#ifndef USE_FASTTRACK_ALG
  clck_t Wvc[TSK_MAX];         // non-fasttrack only
#endif
  int rwId;
} var_state_t;


typedef struct _lock_state_t {
  clck_t lock_vc[TSK_MAX];
} lock_state_t;

////////////////////////////////////////////////////////////////////////////////////////////////

void droidtrack_init();

void unlock(thread_id_t tid, address_t lock_addr);
void droid_lock(thread_id_t tid, address_t lock_addr);

void thread_create(thread_id_t tid, thread_id_t child_tid);
void thread_init(thread_id_t tid, int ext);

void thread_exit(thread_id_t tid);
//TODO
void thread_join(thread_id_t tid, thread_id_t child_tid);

//TODO
void signal();
void broadcast();
void wait();

//TODO
void volatile_access();

void looponq (thread_id_t tid);
void attachq (thread_id_t tid);
void enable (thread_id_t tid, event_id_t event_id);
void post (thread_id_t tid, event_id_t event_id, tsk_id_t tsk_id, thread_id_t dest_tid, 
           int delay, int front/*, int external*/);
void task_begin (thread_id_t tid, tsk_id_t tsk_id);
void task_end (thread_id_t tid, tsk_id_t tsk_id);

void mem_read(thread_id_t tid, int rwId, address_t obj, char *name, int field);
void mem_write(thread_id_t tid, int rwId, address_t obj, char *name, int field);

extern void droidtrack_finish();
extern int consumerStartup();
