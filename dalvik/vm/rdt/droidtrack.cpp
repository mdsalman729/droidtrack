
/****************************************************************************************
 * DroidTrack: On-the-fly Data Race Detection for Event-driven Mobile Programs
 * 
 * Draft v01
 * - When a thread is created, it is treated as one task.
 * - Thus, a thread without event queue is treated as one task.
 * - A thread with an event queue finishes its first task when loop-on-q is called.
 * - Each task has its own unique tsk_id.
 * - The basic algorithm maintains vector clocks for each task (not for each thread).
 *
 *  T1
 *  --tsk-1-begin--
 *  ENABLE-LIFECYCLE
 *  ATTACH-Q
 *  LOOP
 *  --tsk-1-end--
 *
 *  --tsk-2-begin--  <----------------------------------- POST (from system binder thread)
 *  CALL
 *  FORK    ------------------> --tsk-3-begin--
 *  RET                         THREADINIT
 *  --tsk-2-end--               ...
 *                              THREADEXIT
 :*  ...                         --tsk-3-end--
 ***************************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cutils/log.h>
#include "droidtrack.h"
#include <map>
#define READ_SHARED 0xeadbeef
#define DPRINT printf
#define USE_FASTTRACK_ALG
#define TASK_BEGIN  //enabling task_begin_iter_func
#define USE_POST_FRONT
//Salman
std::map<thread_id_t, thread_state_t*> thread_state_map;  // MAP< thread_id_t tid, thread_state_t thrd >
std::map<thread_id_t, thread_state_t*>::iterator thread_it;

std::map<tsk_id_t,task_state_t*> task_state_map;         // MAP< task_id_t tsk_id, task_state_t tsk > 
std::map<tsk_id_t,task_state_t*>::iterator task_it; 

std::map<tsk_id_t,msg_state_t*> msg_state_map;           // MAP< task_id_t tks_id, msg_state_t msg >
std::map<tsk_id_t,msg_state_t*>::iterator msg_it; 
  
std::map<event_id_t,event_state_t*> event_state_map;     // MAP< event_id_t event_id, event_state_t>
std::map<event_id_t,event_state_t*>::iterator event_it; 

std::map<address_t,var_state_t*> var_state_map;          // MAP< address_t var_addr, var_state_t var_stt>
std::map<address_t,var_state_t*>::iterator var_it;

std::map<address_t,lock_state_t*> lock_state_map;        // MAP< address_t lock_addr, lock_state_t lck>
std::map<address_t,lock_state_t*> lock_it;

std::map<thread_id_t,tsk_id_t> reuse_map;                //std::map<thread_id_t,tsk_id_t>::iterator reuse_it;
tsk_id_t g_tsk_id=5;

int g_stats_begin_atom;
int g_stats_begin_eq1;
int g_stats_begin_eq2;
int g_stats_begin_eq3;
int g_stats_begin_eq4;
int g_stats_begin_ext;
int g_stats_begin_nohb;

//Not using droidtrack_finish but this can be called during the onDestroy event() 
void droidtrack_finish() {

  LOGD("g_stats_begin_atom:%d\n",g_stats_begin_atom);
  LOGD("g_stats_begin_eq1:%d\n",g_stats_begin_eq1);
  LOGD("g_stats_begin_eq2:%d\n",g_stats_begin_eq2);
  LOGD("g_stats_begin_eq3:%d\n",g_stats_begin_eq3);
  LOGD("g_stats_begin_eq4:%d\n",g_stats_begin_eq4);
  LOGD("g_stats_begin_ext:%d\n",g_stats_begin_ext);
  LOGD("g_stats_begin_nohb:%d\n",g_stats_begin_nohb);
}

/****************************************************************************************
 * HELPER FUNCTIONS
 ***************************************************************************************/

/* hash: compute hash value of string */
#define MULTIPLIER 31 //31
unsigned int hash_string_to_32bit(char *str)
{
  unsigned int h;
  unsigned char *p;

  h = 0;
  for (p = (unsigned char*)str; *p != '\0'; p++)
    h = MULTIPLIER * h + *p;
  return h; // or, h % ARRAY_SIZE;
}

#define T_MIN(a,b) (((a)<(b))?(a):(b))
#define T_MAX(a,b) (((a)>(b))?(a):(b))

static void vc_join(clck_t *vc1, clck_t *vc2) {
 
 int i;
  for(i=0;i<TSK_MAX;i++){
    vc1[i] = T_MAX (vc1[i], vc2[i]);
  }

}

static void vc_copy(clck_t *vc1, clck_t *vc2) {
  memcpy(vc1,vc2,sizeof(clck_t)*TSK_MAX);
}

static int vc_is_ordered(clck_t *vc1, clck_t *vc2) {
  int ordered = 1;
  
 int i;

  for(i=0;i<TSK_MAX;i++){
    if(vc1[i] > vc2[i]){
      ordered = 0;
      break;
    }
  }
 

  return ordered;
}
/*
static void vc_print(const char *prefix, clck_t *vc){
  int i;
  printf("  %s [",prefix);
  for(i=0;i<TSK_MAX;i++){
    LOGD("%d,",vc[i]);
  }
  LOGD("]\t");
}
*/
tsk_id_t get_tsk_id (thread_id_t tid) {

  thread_state_t *thrd = NULL;

 // NOTE: naive implementation, perhaps using TLS is better idea
 //Salman
  if(thread_state_map.find(tid)==thread_state_map.end()) { 
	LOGD("[ERROR] get_tsk_id(%d): cannot find thread_state_t\n",tid); 
	exit(0);
  }
 thrd=(thread_state_t*)thread_state_map[tid];
  
  return thrd->cur_tsk_id;
}

tsk_id_t get_new_tsk_id(){
 //Salman
 return(++g_tsk_id);

}
/****************************************************************************************
 * UNLOCK -> LOCK
 ***************************************************************************************/

// NOTE: CAFA does NOT include UNLOCK->LOCK
// NOTE: DROIDRACER includes UNLOCK->LOCK only if they run on two different threads

 void unlock(thread_id_t tid, address_t lock_addr){

  tsk_id_t tsk_id = 0;
  task_state_t *tsk = NULL;
  lock_state_t *lck = NULL;

  tsk_id = get_tsk_id(tid);
  //Salman
  if(task_state_map.find(tsk_id)==task_state_map.end()) { 
	LOGD("[ERROR] Cannot find tsk_state_t\n"); 
	exit(0);
  }
 tsk=(task_state_t *)task_state_map[tsk_id];
 LOGD("lock_addr in unlock is %d\n", lock_addr);
 //Salman
 if(lock_state_map.find(lock_addr)==lock_state_map.end()){ 
	LOGD("[ERROR] Cannot find lock_state_t\n"); 
	exit(0);
  }
  lck=(lock_state_t*)lock_state_map[lock_addr];
  
  vc_copy(lck->lock_vc, tsk->vc);

  tsk->vc[tsk_id]++;

  //vc_print("tsk->vc",tsk->vc);
} 

void droid_lock(thread_id_t tid, address_t lock_addr){
  tsk_id_t tsk_id = 0;
  task_state_t *tsk = NULL;
  lock_state_t *lck = NULL;


  tsk_id = get_tsk_id(tid);
  LOGD("Task id %d",tsk_id);
  //Salman
  if(task_state_map.find(tsk_id)==task_state_map.end()){ 
	LOGD("[ERROR] Cannot find tsk_state_t\n"); 
	exit(0);
  }
 tsk=(task_state_t *)task_state_map[tsk_id];
  //Salman
  if(lock_state_map.find(lock_addr)==lock_state_map.end()){ // first lock 
    lck = (lock_state_t *) calloc(sizeof(lock_state_t),1);
    if(lck == NULL) { 
	LOGD("[ERROR] cannot create lock_state_t\n"); 
	exit(0);
    }
  lock_state_map.insert(std::pair<address_t,lock_state_t*>(lock_addr,lck));
  }
  lck=(lock_state_t*)lock_state_map[lock_addr];
  vc_join(tsk->vc, lck->lock_vc);
  //LOGD("Lock value is %d\n",lock_addr);
  //vc_print("tsk->vc",tsk->vc);
} 

/****************************************************************************************
 * FORK -> THREAD_INIT
 ***************************************************************************************/

void thread_create(thread_id_t tid, thread_id_t child_tid){

  tsk_id_t tsk_id = 0;
  task_state_t *tsk = NULL;
  thread_state_t *child_thrd = NULL;

  tsk_id = get_tsk_id(tid);
  //Salman
  if(task_state_map.find(tsk_id)== task_state_map.end()){ 
	LOGD("[ERROR] Cannot find tsk_state_t\n"); 
	exit(0);
  }
  tsk=(task_state_t*)task_state_map[tsk_id];
  child_thrd = (thread_state_t *) calloc(sizeof(thread_state_t),1);
  if(child_thrd == NULL) { 
	LOGD("[ERROR] cannot create thread_state_t\n"); 
	exit(0);
  }
  
  vc_copy(child_thrd->fork_vc, tsk->vc);
   //Salman
  thread_state_map.insert(std::pair<thread_id_t,thread_state_t*>(child_tid,child_thrd));
  tsk->vc[tsk_id]++;

  //vc_print("tsk->vc",tsk->vc);
}

void thread_init(thread_id_t tid, int ext) {
  
  thread_state_t *thrd = NULL;
  task_state_t *tsk = NULL;
  tsk_id_t tsk_id = 0;
  
  //Salman
  if(thread_state_map.find(tid) == thread_state_map.end()) { // first main thread
    thrd = (thread_state_t *) calloc(sizeof(thread_state_t),1);
    if(thrd == NULL) { 
	LOGD("[ERROR] cannot create thread_state_t\n"); 
	exit(0);
    }
    
#ifdef USE_POST_EXTERNAL
    thrd->external = ext; 
#endif
     //Salman
     thread_state_map.insert(std::pair<thread_id_t,thread_state_t*>(tid,thrd));
     LOGD("Added thread %d into the task_map",tid);
  }
  thrd=(thread_state_t*)thread_state_map[tid];
  // a new thread is treated as a task
  tsk_id = get_new_tsk_id(); 

  // new task_state_t
  tsk = (task_state_t *) calloc(sizeof(task_state_t),1);
  if(tsk == NULL) { 
	LOGD("[ERROR] cannot create tsk_state_t\n"); 
	exit(0);
  }
  vc_join(tsk->vc, thrd->fork_vc);  // nop for main thread
  tsk->vc[tsk_id]=1;

  vc_copy(tsk->begin_vc, tsk->vc);
  LOGD("Task id is %d",tsk_id);
  tsk->tid = tid;

  //Salman
  task_state_map.insert(std::pair<tsk_id_t,task_state_t*>(tsk_id,tsk));
  // update cur_tsk_id
  thrd->cur_tsk_id = tsk_id;

  //vc_print("tsk->vc",tsk->vc);
} 

/****************************************************************************************
 * THREAD_EXIT -> JOIN
 ***************************************************************************************/

void thread_exit(thread_id_t tid) {

  thread_state_t *thrd = NULL;
  task_state_t *tsk = NULL;
  tsk_id_t tsk_id = 0;

  tsk_id = get_tsk_id(tid);

  //Salman
  if(thread_state_map.find(tid) == thread_state_map.end()){ 
	LOGD("[ERROR] Cannot find thread_state_t\n"); 
	exit(0);
  }
  //Salman
  thread_it=thread_state_map.find(tid);
  thread_state_map.erase(thread_it);
  free(thrd);
  LOGD("thread exits %d",tid);
  if(task_state_map.find(tsk_id) == task_state_map.end()){ 
	LOGD("[ERROR] Cannot find tsk_state_t\n"); 
	exit(0);
  }
  //Salman
  task_it=task_state_map.find(tsk_id);
  task_state_map.erase(task_it);
  free(tsk);

} 

void thread_join(thread_id_t tid, thread_id_t child_tid){

  tsk_id_t tsk_id = 0;
  tsk_id_t child_tsk_id = 0;
  task_state_t *tsk = NULL;
  task_state_t *child_tsk = NULL;
  tsk_id = get_tsk_id(tid);
  child_tsk_id = get_tsk_id(child_tid);

  //Salman
  if(task_state_map.find(tsk_id) == task_state_map.end()) { 
	LOGD("[ERROR] Cannot find tsk_state_t\n"); 
	exit(0);
  }
  tsk=(task_state_t*)task_state_map[tsk_id];
  if(task_state_map.find(child_tsk_id) == task_state_map.end()){ 
	LOGD("[ERROR] Cannot find tsk_state_t\n"); 
	exit(0);
  }
  child_tsk=(task_state_t*)task_state_map[child_tsk_id];
  vc_join(tsk->vc, child_tsk->vc);

  //vc_print("tsk->vc",tsk->vc);
}

/****************************************************************************************
 * SIGNAL -> WAIT
 ***************************************************************************************/

void signal() {
  //TODO
}

void broadcast() {
  //TODO
}

void wait(){
  //TODO
}

/****************************************************************************************
 * VOLATILE ACCESS
 ***************************************************************************************/

void volatile_access() {
  //TODO
}

/****************************************************************************************
 * LOOPONQ (END-like)
 * 
 * ATTACHQ -> POST
 * ENABLE -> POST
 *
 * POST -> BEGIN
 * END -> BEGIN (NOPRE/Atomicity rule)
 * END -> BEGIN (FIFO/Event queue rule)
 * END -> BEGIN (External input rule)
 ***************************************************************************************/

void looponq (thread_id_t tid){

  tsk_id_t tsk_id = 0;
  thread_state_t *thrd = NULL;
  tsk_id = get_tsk_id(tid);

  //Salman
  if(thread_state_map.find(tid) == thread_state_map.end()) { 
	LOGD("[ERROR] cannot find thread_state_t\n"); 
	exit(0);
  }
  thrd=(thread_state_t*)thread_state_map[tid];

  if(thrd->prev_tsk_q == NULL){
    thrd->prev_tsk_q = ds_queue_alloc();
  }
  LOGD("loop on q Thread id is %d",tid);
  ds_queue_enqueue(thrd->prev_tsk_q, (void *)(intptr_t)tsk_id);

  //Salman
  if(task_state_map.find(tsk_id) == task_state_map.end()){ 
	LOGD("[ERROR] Cannot find tsk_state_t\n"); 
	exit(0);
  }
  //task_state_t *tsk=(task_state_t *)task_state_map[tsk_id];
  //vc_print("tsk->vc",tsk->vc);
}

void attachq (thread_id_t tid){

  thread_state_t *thrd = NULL;
  task_state_t *tsk = NULL;
  tsk_id_t tsk_id = 0;
  tsk_id = get_tsk_id(tid);

  //Salman
  if(task_state_map.find(tsk_id) == task_state_map.end()){ 
	LOGD("[ERROR] Cannot find tsk_state_t\n"); 
	exit(0);
  }
  tsk=(task_state_t*)task_state_map[tsk_id];
  //Salman
  if(thread_state_map.find(tid) == thread_state_map.end()) { 
	LOGD("[ERROR] cannot find thread_state_t\n"); 
	exit(0);
  }
  thrd=(thread_state_t*)thread_state_map[tid];
  vc_copy(thrd->attachq_vc, tsk->vc);

  tsk->vc[tsk_id]++; 
  vc_copy(tsk->begin_vc, tsk->vc);  

  //vc_print("tsk->vc",tsk->vc);
}

// NOTE: The current DROIDRACER trace does not include event_id
//
void enable (thread_id_t tid, event_id_t event_id){
 
tsk_id_t tsk_id = 0;
#ifdef USE_EVENT_ENABLE
  task_state_t *tsk=NULL;
  event_state_t *evt = NULL;
#endif

  tsk_id = get_tsk_id(tid);
  //LOGD("Present task executing %d",tsk_id);
  //Salman
  if(task_state_map.find(tsk_id) == task_state_map.end()){ 
	LOGD("[ERROR] Cannot find tsk_state_t\n"); 
	exit(0);
  }

#ifdef USE_EVENT_ENABLE
  tsk=(task_state_t*)task_state_map[tsk_id];
  // new event_state_t
  evt = calloc(sizeof(event_state_t),1);
  if(evt == NULL) { 
	LOGD("[ERROR] cannot create event_state_t\n"); 
	exit(0);
  }
  //Salman
   event_state_map.insert(std::pair<event_id_t,event_state_t*>(event_id,evt));

  vc_copy(evt->enable_vc, tsk->vc);

  tsk->vc[tsk_id]++;
  LOGD("Task vector clock is %d and task id is %d",tsk->vc[tsk_id],tsk_id);
  //vc_print("tsk->vc",tsk->vc);
#endif

}

// NOTE: The current DROIDRACER trace does not include event_id 
// NOTE: tsk_id == msg_id

void post (thread_id_t tid, event_id_t event_id, tsk_id_t dest_tsk_id, thread_id_t dest_tid, int delay, int front/*, int external*/){

  thread_state_t *thrd = NULL;
  thread_state_t *dest_thrd = NULL;
  task_state_t *tsk = NULL;
  msg_state_t *msg = NULL;
#ifdef USE_EVENT_ENABLE
  event_state_t *evt = NULL;
#endif
  tsk_id_t tsk_id = 0;

  //Salman
  if(thread_state_map.find(tid) == thread_state_map.end()) { // Posted by non-app threads (e.g., binder thread)
    // emulate thread_init 
    thread_init(tid,1);
    thrd = (thread_state_t*)thread_state_map[tid];
  }
  thrd=(thread_state_t*)thread_state_map[tid];
  tsk_id = get_tsk_id(tid);

  tsk = (task_state_t*)task_state_map[tsk_id];
  //Salman
  if(task_state_map.find(tsk_id) == task_state_map.end()) { 
	LOGD("[ERROR] Cannot find tsk_state_t\n"); 
	exit(0);
  }
  dest_thrd = (thread_state_t*)thread_state_map[dest_tid];
  //Salman
  if(thread_state_map.find(dest_tid) == thread_state_map.end()) { 
	LOGD("[ERROR] cannot find thread_state_t\n"); 
	exit(0);
  }
 LOGD("thread is is %d", tsk->tid);
  // ATTACHQ -> POST
  vc_join( tsk->vc, dest_thrd->attachq_vc);
  
#ifdef USE_EVENT_ENABLE
  //evt = ds_hash_find(event_state_map,event_id);
  //Salman
  if(event_state_map.find(event_id) == event_state_map.end()) { 
	LOGD("[ERROR] Cannot find event_state_t\n"); 
	exit(0);
  }
  evt=(event_state_t*)event_state_map[event_id];
  // ENABLE -> POST
  vc_join( tsk->vc, evt->enable_vc);
#endif

  // POST -> BEGIN
  
  //new msg_state_t;
  msg = (msg_state_t*) calloc(sizeof(msg_state_t),1);
  if(msg == NULL) { 
	LOGD("[ERROR] cannot create msg_state_t\n"); 
   	exit(0);
  }

  msg->src_tid = tid;
  vc_copy(msg->post_vc, tsk->vc);
  msg->delay = delay;
#ifdef USE_POST_FRONT
  msg->front = front;
#endif
#ifdef USE_POST_EXTERNAL
  msg->external = thrd->external;
#endif
 //Salman
  msg_state_map.insert(std::pair<tsk_id_t,msg_state_t*>(dest_tsk_id,msg)); 

  tsk->vc[tsk_id]++; 

 //vc_print("tsk->vc",tsk->vc);
}

// NOTE: tsk_id == msg_id
// TODO: perhaps we do not need to keep all prev tsks ...
#ifdef TASK_BEGIN
static void *task_begin_iter_func (void *queue_data, void *user_data1, void *user_data2){
  //Previous task and ordering between end and begin of task is checked to reuse entire vc.
  tsk_id_t prev_tsk_id = (tsk_id_t)(intptr_t)queue_data;
  task_state_t *tsk = (task_state_t *)user_data1;
  msg_state_t *msg = (msg_state_t *)user_data2;
  int is_ordered = 0;
  int is_opt=0;
  task_state_t *prev_tsk = NULL;
  msg_state_t *prev_msg = NULL;
  tsk_id_t tsk_id=0;
  //Salman
  //LOGD("Previous task id is %d",prev_tsk_id);
  if(task_state_map.find(prev_tsk_id) == task_state_map.end()){ 
	LOGD("[ERROR] Cannot find tsk_state_t\n"); 
	//exit(0);
        return 0;
  }
  prev_tsk=(task_state_t*)task_state_map[prev_tsk_id];
  // NOPRE/Atomicity rule
  
  for(msg_it=msg_state_map.begin();msg_it!=msg_state_map.end();msg_it++){
           if(msg_it->second==msg)
             tsk_id=msg_it->first;
  }   
  if(reuse_map.find(msg->src_tid)==reuse_map.end()){ //Checking if our thread is present before, ie there exists a task before on the same logical thread
         reuse_map.insert(std::pair<thread_id_t,tsk_id_t>(msg->src_tid,tsk_id)); //incase none exists insert into the map.
         LOGD("New thread adding an entry into the map structure\n");
  }
  else{
         prev_tsk_id=reuse_map[msg->src_tid]; //incase it exisis find out the prev task id presently i am just finding out the first task.
         //LOGD("This is the new optimization that we have done \n");
         if(prev_tsk_id!=tsk_id)
          is_opt=1;
  }

  if(vc_is_ordered(prev_tsk->begin_vc, msg->post_vc) && is_opt==0){ //checking if bookkeeper is already running 
    LOGD("  Atom rule: end(tsk_id:%d)->begin()\n",prev_tsk_id);
    g_stats_begin_atom++;
    vc_join(tsk->vc, prev_tsk->vc);
    is_ordered = 1;
  } 
  else {

   //Salman   
   if(msg_state_map.find(prev_tsk_id) == msg_state_map.end()) { 
	return 0;
      }
  prev_msg=(msg_state_t*)msg_state_map[prev_tsk_id];

  if(is_opt==1) {
      prev_tsk=(task_state_t*)task_state_map[prev_tsk_id];
      //Here I am finding out the task id of the first task in the thread. I think as long as there is no delay involved we can always use the first one and keep only one entry per thread
      //atleast for the FIFO rule.
      LOGD("Reaching fifo optimizer\n");
      vc_join(tsk->vc,prev_tsk->vc);
      is_ordered=1;
     } 
   else if(vc_is_ordered(prev_msg->post_vc, msg->post_vc) && prev_msg->delay <= msg->delay) {
      //printf("  EQ rule 1: end(tsk_id:%d)->begin()\n",prev_tsk_id);
      g_stats_begin_eq1++;
      vc_join(tsk->vc, prev_tsk->vc);
      is_ordered = 1;
      tsk->parent_tsk_id=prev_tsk_id;
      LOGD("Ordering due to delay\n");
      LOGD("Using the following previous task id %d",prev_tsk_id);
      //LOGD("Previous message source thread is %d and present message source thread id is %d",prev_msg->src_tid,msg->src_tid);
    }
#ifdef USE_POST_FRONT
    // (CAFA) event queue rule 2    
    else if(vc_is_ordered(msg->post_vc, prev_msg->post_vc) && prev_msg->front && vc_is_ordered(prev_msg->post_vc, tsk->vc)){
      LOGD("  EQ rule 2: end(tsk_id:%d)->begin()\n",prev_tsk_id);
      g_stats_begin_eq2++;
      vc_join(tsk->vc, prev_tsk->vc);
      is_ordered = 1;
    }
    // (CAFA) event queue rule 3
    else if(vc_is_ordered(prev_msg->post_vc, msg->post_vc) && prev_msg->front){
      LOGD("  EQ rule 3: end(tsk_id:%d)->begin()\n",prev_tsk_id);
      g_stats_begin_eq3++;
      vc_join(tsk->vc, prev_tsk->vc);
      is_ordered = 1;
    }
    // (CAFA) event queue rule 4    
    else if(vc_is_ordered(msg->post_vc, prev_msg->post_vc) && prev_msg->front && msg->front && vc_is_ordered(prev_msg->post_vc, tsk->vc)){
      LOGD("  EQ rule 4: end(tsk_id:%d)->begin()\n",prev_tsk_id);
      g_stats_begin_eq4++;
      vc_join(tsk->vc, prev_tsk->vc);
      is_ordered = 1;
    }
#endif
#ifdef USE_POST_EXTERNAL
    // (CAFA) external input rule 
    else if(prev_msg->external && msg->external){ 
      LOGD("  Ext. rule: end(tsk_id:%d)->begin()\n",prev_tsk_id);
      g_stats_begin_ext++;
      vc_join(tsk->vc, prev_tsk->vc);
      is_ordered = 1;
    }
#endif
    // not ordered
    else{
      LOGD("  No HB: end(tsk_id:%d)-x->begin()\n",prev_tsk_id);
      LOGD("previous task's source thread is equal to %d",prev_msg->src_tid);
      g_stats_begin_nohb++;
    }
 }
   
  if(is_ordered)
    return prev_tsk;
  else
    return 0;
}
#endif
void task_begin (thread_id_t tid, tsk_id_t tsk_id){

  thread_state_t *thrd = NULL;
  task_state_t *tsk = NULL;
  msg_state_t *msg = NULL;
  task_state_t *prev_tsk = NULL;
  //Salman
  if(thread_state_map.find(tid) == thread_state_map.end()) { 
	LOGD("[ERROR] cannot find thread_state_t\n"); 
	exit(0);
  }
  thrd=(thread_state_t*)thread_state_map[tid];
  thrd->cur_tsk_id = tsk_id;

  // create the new task
  tsk = (task_state_t*) calloc(sizeof(task_state_t),1);
  if(tsk == NULL) { 
	LOGD("[ERROR] cannot create tsk_state_t\n"); 
       	exit(0);
  }
  tsk->tid = tid;
 //Salman 
  task_state_map.insert(std::pair<tsk_id_t,task_state_t*>(tsk_id,tsk));
 
 // POST -> BEGIN
 //Salman
  if(msg_state_map.find(tsk_id) == msg_state_map.end()) { 
	LOGD("[ERROR] cannot find msg_state_t\n"); 
	exit(0);
  }                         
  msg=(msg_state_t*)msg_state_map[tsk_id];
  vc_copy(tsk->vc, msg->post_vc);
  tsk->vc[tsk_id]=1;

  if(thrd->prev_tsk_q == NULL){ 
	LOGD("[ERROR] no prev_tsk_q\n"); 
 }

if(thrd->prev_tsk_q!=NULL)
  {  
	prev_tsk = (task_state_t*) ds_queue_iterate_stop(thrd->prev_tsk_q, task_begin_iter_func, tsk, msg,tsk_id);
  }
  
  if(prev_tsk)  // END->BEGIN
   { 
   	vc_copy(tsk->begin_vc, prev_tsk->vc); 
        LOGD("cuda Yes we are reusing the vector clocks\n");
   } 
  else
    vc_copy(tsk->begin_vc, tsk->vc);
 //LOGD("Task id is %d",tsk_id);
 // vc_print("tsk->vc",tsk->vc);

}

void task_end (thread_id_t tid, tsk_id_t tsk_id){

  thread_state_t *thrd = NULL;
  //Salman
  if(thread_state_map.find(tid) == thread_state_map.end()) { 
	LOGD("[ERROR] cannot find thread_state_t\n"); 
	exit(0);
  }
  thrd=(thread_state_t*)thread_state_map[tid];
  
 if(thrd->prev_tsk_q == NULL){ 
	LOGD("[ERROR] no prev_tsk_q\n"); 
	//exit(0);
  }

 if(thrd->prev_tsk_q!=NULL){ 
 ds_queue_enqueue(thrd->prev_tsk_q, (void *)(intptr_t)tsk_id);
}
  //Salman
  if(task_state_map.find(tsk_id) == task_state_map.end()){ 
	LOGD("[ERROR] Cannot find tsk_state_t\n"); 
	exit(0);
  }
  //vc_print("tsk->vc",tsk->vc);
}

/****************************************************************************************
 * READ and WRITE checks
 ***************************************************************************************/

void mem_read(thread_id_t tid, int rwId, address_t obj, char *name, int field){

  tsk_id_t tsk_id = 0;
  task_state_t *tsk = NULL;
  var_state_t *var = NULL;
  address_t var_addr = 0;
  char var_string[256] = {0,};

  tsk_id = get_tsk_id(tid);
  //Salman
  if(task_state_map.find(tsk_id) == task_state_map.end()){ 
	LOGD("[ERROR] Cannot find tsk_state_t\n"); 
	exit(0);
  }
  tsk=(task_state_t*)task_state_map[tsk_id];
  // take hash
  sprintf(var_string,"%d:%s:%d",obj,name,field);
  var_addr = hash_string_to_32bit(var_string);
#ifdef USE_FASTTRACK_ALG
  if(var_state_map.find(var_addr) == var_state_map.end()){ // first access
    var = (var_state_t*)calloc(sizeof(var_state_t),1);
    if(var == NULL) { 
	LOGD("[ERROR] cannot create var_state_t\n"); 
	exit(0);
    }

    LOGD("Value of tsk is %d",tsk->vc[tsk_id]);
    var->R = tsk->vc[tsk_id]; // tsk->epoch
    var->R_tsk_id = tsk_id;
    var->rwId = rwId;
    //Salman
      var_state_map.insert(std::pair<address_t,var_state_t*>(var_addr,var));
  }else{
    var=(var_state_t*)var_state_map[var_addr];
    // same epoch
    if(var->R == tsk->vc[tsk_id]){
      return;
    }

    // write-read race?
    if(var->W > tsk->vc[var->W_tsk_id]){
      LOGD("[DATA RACE] W(rwId:%d)-R(rwId:%d) obj- %d class- %s\n",var->rwId,rwId, obj,name);
    }

    // update read state
    if(var->R == READ_SHARED) {            // shared
      var->Rvc[tsk_id] = tsk->vc[tsk_id];
    } else {
      if(var->R <= tsk->vc[var->R_tsk_id]){   // exclusive
        var->R = tsk->vc[tsk_id];
        var->R_tsk_id = tsk_id;
      }else{                              // (slow path)
        var->Rvc[var->R_tsk_id] = var->R;
        var->Rvc[tsk_id] = tsk->vc[tsk_id];
        var->R = READ_SHARED;
      }
    }

    var->rwId = rwId;
  }
#else
  if(var == NULL){ // first access
    var = (var_state_t*) calloc(sizeof(var_state_t),1);
    if(var == NULL) { 
	LOGD("[ERROR] cannot create var_state_t\n"); 
	exit(0);
     }
    vc_copy(var->Rvc, tsk->vc);
    var_state_map.insert(std::pair<address_t,var_state_t*>(var_addr,var));
  }else{

    if(!vc_is_ordered(var->Wvc,tsk->vc)){
      LOGD("[DATA RACE] W(rwId:%d)-R(rwId:%d)\n",var->rwId,rwId);
      //vc_print("var->W ",var->Wvc);
      //vc_print("tsk->vc",tsk->vc);
      //exit(0);
    }

    // update read state
    vc_copy(var->Rvc, tsk->vc);

    var->rwId = rwId;
  }
#endif
}

void mem_write(thread_id_t tid, int rwId, address_t obj, char *name, int field){

  tsk_id_t tsk_id = 0;
  task_state_t *tsk = NULL;
  var_state_t *var = NULL;
  address_t var_addr = 0;
  char var_string[256] = {0,};
  tsk_id = get_tsk_id(tid);
  //LOGD("Task id is %d",tsk_id);
  if(task_state_map.find(tsk_id) == task_state_map.end()){ 
	LOGD("[ERROR] Cannot find tsk_state_t\n"); 
	exit(0);
  }
  tsk=(task_state_t*)task_state_map[tsk_id];
  // take hash
  sprintf(var_string,"%d:%s:%d",obj,name,field);
  var_addr = hash_string_to_32bit(var_string);
  //Salman
#ifdef USE_FASTTRACK_ALG
  if(var_state_map.find(var_addr) == var_state_map.end()){ // first access
    var = (var_state_t*)calloc(sizeof(var_state_t),1);
    if(var == NULL) { 
	LOGD("[ERROR] cannot create var_state_t\n"); 
	exit(0);
    }
  var = (var_state_t*)var_state_map[var_addr];
  LOGD("WRITE task id is %d",tsk->vc[tsk_id]);
  var->W = tsk->vc[tsk_id]; // tsk->epoch
  //LOGD("task id is %d", tsk_id);
  var->W_tsk_id = tsk_id;
  var->rwId = rwId;
  var_state_map.insert(std::pair<address_t,var_state_t*>(var_addr,var));
  }else{
    // same epoch
  LOGD("Task id is %d",tsk->vc[tsk_id]);
    
  var=(var_state_t*)var_state_map[var_addr];
  LOGD("Variable clock is %d",var->W);
  if(var->W == tsk->vc[tsk_id]){
      return;
  }


    // write-write race?
    if(var->W > tsk->vc[var->W_tsk_id]){
      LOGD("[DATA RACE] W(rwId:%d)-W(rwId:%d) obj %d class %s\n",var->rwId,rwId,obj,name);
      //exit(0);
    }

    // read-write race?
    if(var->R != READ_SHARED) {              // shared
      if (var->R > tsk->vc[var->R_tsk_id]){
        LOGD("[DATA RACE] R(rwId:%d)-W(rwId:%d) obj %d class %s\n",var->rwId,rwId,obj,name);
        //exit(0);
      }
    }else{                                  // exclusive
      if (!vc_is_ordered(var->Rvc,tsk->vc)){  // (slow path)
        LOGD("[DATA RACE] R(rwId:%d)-W(rwId:%d) obj %d class %s\n",var->rwId,rwId,obj,name);
        //exit(0);
      }
    }

    // update write state
    var->W = tsk->vc[tsk_id]; // tsk->epoch
    var->W_tsk_id = tsk_id;
    var->rwId = rwId;
  }
#else
  if(var == NULL){ // first access
    var = (var_state_t*) calloc(sizeof(var_state_t),1);
    if(var == NULL) { 
	LOGD("[ERROR] cannot create var_state_t\n"); 
	exit(0);
    }

    vc_copy(var->Wvc, tsk->vc);

var_state_map.insert(std::pair<address_t,var_state_t*>(var_addr,var));
  }else{

    if(!vc_is_ordered(var->Wvc,tsk->vc)){
      LOGD("[DATA RACE] W(rwId:%d)-W(rwId:%d)\n",var->rwId,rwId);
      //vc_print("var->W ",var->Wvc);
      //vc_print("tsk->vc",tsk->vc);
      exit(0);
    }

    if(!vc_is_ordered(var->Rvc,tsk->vc)){
      LOGD("[DATA RACE] R(rwId:%d)-W(rwId:%d)\n",var->rwId,rwId);
      //vc_print("var->R ",var->Rvc);
      //vc_print("tsk->vc",tsk->vc);
      exit(0);
    }

    // update write state
    vc_copy(var->Wvc, tsk->vc);

    var->rwId = rwId;
  }
#endif
}
