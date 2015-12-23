#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//Salman
//#include "queue.h"
#include "droidtrack.h"
//Salman
#include "queue.h"
//extern wqueue<WorkItem*> *q;
int main(int argc,char **argv)
{
  char line_token[512];
  char line[512];
  //char *seps = " \n,( )";
  char *seps = " \n";
  char *p;
  FILE *f;
  //Salman
  //WorkItem* item;

  if(argc != 2) {
    printf("[ERROR] usage: %s <trace>\n",argv[0]);
    return -1;
  }

  f=fopen(argv[1],"r");
  if(!f) {
    return -1;
  }

  //init
  droidtrack_init();
  //Salman
  //wqueueInit();
 // consumerStartup();

  //parse and process
  while(fgets( line, sizeof(line)-1, f) != NULL)
  {
    strcpy(line_token,line);

    // 
    p = strtok( line_token, seps ); 
    if(p != NULL){
      //int id = 0;
      //id = atoi(p);
      //if(id==0) continue;
      if(strcmp(p,"NEW")==0 || strcmp(p,"METHOD")==0) continue;
    }

    // TYPE
    p = strtok( NULL, seps ); /* Find next token*/
    if(p != NULL){

/*
void unlock(thread_id_t tid, address_t lock_addr);
void lock(thread_id_t tid, address_t lock_addr);

void fork(thread_id_t tid, thread_id_t child_tid);
void threadinit(thread_id_t tid);

void thread_exit();
void join(thread_id_t tid, thread_id_t child_tid);

void signal();
void broadcast();
void wait();

void volatile_access();
*/
      if(strcmp(p,"UNLOCK")==0){
        //39 UNLOCK tid:1	 lock-obj:0x41065378
        thread_id_t tid = 0;
        address_t lock_addr = 0;

        if(sscanf(line,"%*s UNLOCK tid:%d lock-obj:%X",&tid,&lock_addr)!=2){
          printf("[ERROR] UNLOCK parsing fails\n");
          return -1;
        }
	//Jay: Create a workitem and add into the queue
	//WorkItem(int name, int tid, int ctid, int dest_tid, int tsk,int lck_addr, int dely, int rwid, int obj, char *class_name, char *field_name)
//Salman
	//item =  new WorkItem(UNLOCK, tid, -1,-1,-1,lock_addr,-1,-1,-1,NULL, NULL);
	//q->add(item);
        unlock(tid,lock_addr);
        LOGD("RCVD UNLOCK \n");
	//Jay: This needs to be moved to the worker thread
        //unlock(tid,lock_addr);
      }
      else if(strcmp(p,"LOCK")==0){
        //37 LOCK tid:1	 lock-obj:0x41065378
        thread_id_t tid = 0;
        address_t lock_addr = 0;

        if(sscanf(line,"%*s LOCK tid:%d lock-obj:%X",&tid,&lock_addr)!=2){
          printf("[ERROR] LOCK parsing fails\n");
          return -1;
        }
	/*item =  new WorkItem(LOCK, tid, -1,-1,-1,lock_addr,-1,-1,-1,NULL, NULL);
	q->add(item);*/
//Salman
        droid_lock(tid,lock_addr);
        LOGD("RCVD LOCK \n");

        //lock(tid,lock_addr);
      }
      else if(strcmp(p,"FORK")==0){
        //349 FORK par-tid:1	 child-tid:10
        thread_id_t tid = 0;
        thread_id_t child_tid = 0;

        if(sscanf(line,"%*s FORK par-tid:%d child-tid:%d",&tid,&child_tid)!=2){
          printf("[ERROR] FORK parsing fails\n");
          return -1;
        }
	/*item =  new WorkItem(FORK, tid, child_tid,-1,-1,-1,-1,-1,-1,NULL, NULL);
	q->add(item);*/
 //Salman
        thread_create(tid,child_tid); 
        LOGD("RCVD FORK \n");
        //thread_create(tid,child_tid);
      }
      else if(strcmp(p,"THREADINIT")==0){
        //2 THREADINIT tid:1
        thread_id_t tid = 0;

        if(sscanf(line,"%*s THREADINIT tid:%d",&tid)!=1){
          printf("[ERROR] THREADINIT parsing fails\n");
          return -1;
        }
	/*item =  new WorkItem(THREADINIT, tid, -1,-1,-1,-1,-1,-1,-1,NULL, NULL);
	q->add(item);*/
//Salman
        thread_init(tid, 0); 
        LOGD("RCVD THREADINIT \n");
        //thread_init(tid, 0);
      }
      else if(strcmp(p,"THREADEXIT")==0){
        //35 THREADEXIT tid:10
        thread_id_t tid = 0;

        if(sscanf(line,"%*s THREADEXIT tid:%d",&tid)!=1){
          printf("[ERROR] THREADEXIT parsing fails\n");
          return -1;
        }
	/*item =  new WorkItem(THREADEXIT, tid, -1,-1,-1,-1,-1,-1,-1,NULL, NULL);
	q->add(item);*/
//Salman
        thread_exit(tid);
        LOGD("RCVD THREADEXIT \n");
        //thread_exit(tid);
      }
      else if(strcmp(p,"JOIN")==0){
        //TODO: NO EXAMPLE
        thread_id_t tid = 0;
        thread_id_t child_tid = 0;

        if(sscanf(line,"%*s JOIN par-tid:%d child-tid:%d",&tid,&child_tid)!=2){
          printf("[ERROR] JOIN parsing fails\n");
          return -1;
        }
	/*item =  new WorkItem(JOIN, tid, child_tid,-1,-1,-1,-1,-1,-1,NULL, NULL);
	q->add(item);*/
//Salman
	thread_join(tid,child_tid);
        LOGD("RCVD JOIN \n"); 
        //thread_join(tid,child_tid);
      }
      else if(strcmp(p,"LOOP")==0){
        //14 LOOP tid:1	 queue:1090913792
        thread_id_t tid = 0;

        if(sscanf(line,"%*s LOOP tid:%d",&tid)!=1){
          printf("[ERROR] LOOP parsing fails\n");
          return -1;
        }
	/*item =  new WorkItem(LOOP, tid, -1,-1,-1,-1,-1,-1,-1,NULL, NULL);
	q->add(item);*/
//Salman
	looponq(tid);
	LOGD("RCVD LOOP \n");
        //looponq(tid);
      }
      else if(strcmp(p,"ATTACH-Q")==0){
        //4 ATTACH-Q tid:1	 queue:1090913792
        thread_id_t tid = 0;

        if(sscanf(line,"%*s ATTACH-Q tid:%d",&tid)!=1){
          printf("[ERROR] ATTACH-Q parsing fails\n");
          return -1;
        }
	/*item =  new WorkItem(ATTACHQ, tid, -1,-1,-1,-1,-1,-1,-1,NULL, NULL);
	q->add(item);*/
//Salman
	attachq(tid);
	LOGD("RCVD ATTACHQ \n");
        //attachq(tid);
      }
      else if(strcmp(p,"ENABLE-LIFECYCLE")==0){
        //3 ENABLE-LIFECYCLE tid:1 component: id:-1 state:BIND-APP
        thread_id_t tid = 0;

        if(sscanf(line,"%*s ENABLE-LIFECYCLE tid:%d",&tid)!=1){
          printf("[ERROR] ENABLE-LIFECYCLE parsing fails\n");
          return -1;
        }
	/*item =  new WorkItem(ENABLELIFECYCLE, tid, -1,-1,-1,-1,-1,-1,-1,NULL, NULL);
	q->add(item);*/
//Salman
	enable(tid,NA);
	LOGD("RCVD ENABLELIFECYCLE \n");
      }
      else if(strcmp(p,"ENABLE-EVENT")==0){
        //27 ENABLE-EVENT tid:1 view:1091306944 event:0
        thread_id_t tid = 0;

        if(sscanf(line,"%*s ENABLE-EVENT tid:%d",&tid)!=1){
          printf("[ERROR] ENABLE-EVENT parsing fails\n");
          return -1;
        }

	//item =  new WorkItem(ENABLEEVENT, tid, -1,-1,-1,-1,-1,-1,-1,NULL, NULL);
	//q->add(item);
//Salman
        enable(tid,NA);
	LOGD("RCVD ENABLEEVENT \n");
      }
      else if(strcmp(p,"POST")==0){
        //6 POST src:8 msg:1 dest:-1 delay:0
        thread_id_t tid = 0;
        thread_id_t dst_tid = 0;
        tsk_id_t tsk_id = 0;
        int delay = 0;

        if(sscanf(line,"%*s POST src:%d msg:%d dest:-%d delay:%d",&tid,&tsk_id,&dst_tid,&delay)!=4){
          printf("[ERROR] POST parsing fails:%s\n",line);
          return -1;
        }
        //Jay:WorkItem(int name, int tid, int ctid, int dest_tid, int tsk,int lck_addr, int dely, int rwid, int obj, char *class_name, char *field_name)
	/*item =  new WorkItem(POST, tid, -1,dst_tid,tsk_id,-1,delay,-1,-1,NULL, NULL);
	q->add(item);*/
//Salman
 	LOGD("RCVD POST \n"); 
	int first;
	if(item->delay == -1){
                          first = 1;
                          item->delay = 0;
                }
                        else {
                          first = 0;
                        }
        post (tid,NA,tsk_id,dest_tid,delay,first);

        //TODO: event_id, front, external
        //post (tid, NA, tsk_id, dst_tid, delay, NA/*, NA*/);
      }
      else if(strcmp(p,"CALL")==0){
        //15 CALL tid:1	 msg:1
        thread_id_t tid = 0;
        tsk_id_t tsk_id = 0;

        if(sscanf(line,"%*s CALL tid:%d\tmsg:%d",&tid,&tsk_id)!=2){
          printf("[ERROR] CALL parsing fails\n");
          return -1;
        }
	/*item =  new WorkItem(CALL, tid, -1,-1,tsk_id,-1,-1,-1,-1,NULL, NULL);
	q->add(item);*/
//Salman
	task_begin(tid,tsk_id); 
        LOGD("RCVD CALLMETHOD \n");

        //task_begin(tid,tsk_id);
      }
      else if(strcmp(p,"RET")==0){
        //16 RET tid:1	 msg:1
        thread_id_t tid = 0;
        tsk_id_t tsk_id = 0;

        if(sscanf(line,"%*s RET tid:%d msg:%d",&tid,&tsk_id)!=2){
          printf("[ERROR] RET parsing fails:%s\n",line);
          return -1;
        }
	//item =  new WorkItem(RET, tid, -1,-1,tsk_id,-1,-1,-1,-1,NULL, NULL);
	//q->add(item);
//Salman
	task_end(tid,tsk_id); 
        LOGD("RCVD RET \n");
        //task_end(tid,tsk_id);
      }
      else if(strcmp(p,"READ")==0){
        //rwId:569 READ tid:1 obj:0x41065378 class:Landroid/app/SharedPreferencesImpl; field:28
        //rwId:962 READ tid:1 database:/data/data/org.tomdroid/databases/tomdroid-notes.db
	//Jay:WorkItem(int name, int tid, int ctid, int dest_tid, int tsk,int lck_addr, int dely, int rwid, int obj, char *class_name, char *field_name)
        int rwId = 0;
        thread_id_t tid = 0;
        address_t obj = 0;
        char name[128] = {0,};
        char field[128] = {0,};

        if(sscanf(line,"rwId:%d READ tid:%d obj:%X class:%s field:%s",&rwId,&tid,&obj,name,field)!=5){
        if(sscanf(line,"wId:%d READ tid:%d obj:%X class:%s field:%s",&rwId,&tid,&obj,name,field)!=5){
        if(sscanf(line,"MrwId:%d READ tid:%d obj:%X class:%s field:%s",&rwId,&tid,&obj,name,field)!=5){
        if(sscanf(line,"MwId:%d READ tid:%d obj:%X class:%s field:%s",&rwId,&tid,&obj,name,field)!=5){
          if(sscanf(line,"rwId:%d READ tid:%d database:%s",&rwId,&tid,name)!=3){
          if(sscanf(line,"wId:%d READ tid:%d database:%s",&rwId,&tid,name)!=3){
          if(sscanf(line,"MrwId:%d READ tid:%d database:%s",&rwId,&tid,name)!=3){
            printf("[ERROR] READ parsing fails - %s\n",line);
            return -1;
          }}}
        }}}}

	/*item =  new WorkItem(READ, tid, -1,-1,-1,-1,-1,rwId,obj,name, field);
	q->add(item);*/
//Salman
	mem_read(tid,rwId,obj,name,field);
	LOGD("RCVD READ \n");
        
      }
      else if(strcmp(p,"READ-STATIC")==0){
        //rwId:567 READ-STATIC tid:1 class:Lorg/tomdroid/util/Preferences; field:320
        int rwId = 0;
        thread_id_t tid = 0;
        address_t obj = 0;
        char name[128] = {0,};
        char field[128] = {0,};

        if(sscanf(line,"rwId:%d READ-STATIC tid:%d class:%s field:%s",&rwId,&tid,name,field)!=4){
        if(sscanf(line,"wId:%d READ-STATIC tid:%d class:%s field:%s",&rwId,&tid,name,field)!=4){
        if(sscanf(line,"MrwId:%d READ-STATIC tid:%d class:%s field:%s",&rwId,&tid,name,field)!=4){
        if(sscanf(line,"MwId:%d READ-STATIC tid:%d class:%s field:%s",&rwId,&tid,name,field)!=4){
          printf("[ERROR] READ parsing fails - %s\n",line);
          return -1;
        }}}}

	//item =  new WorkItem(READSTATIC, tid, -1,-1,-1,-1,-1,rwId,obj,name, field);
	//q->add(item);
//Salman
        mem_read(tid,rwId,obj,name,field);
	LOGD("RCVD READ \n");
      }
      else if(strcmp(p,"WRITE")==0){
        //rwId:579 WRITE tid:1 obj:0x41095bc0 class:Lorg/tomdroid/Note; field:32
        //rwId:7340 WRITE tid:11 database:/data/data/org.tomdroid/databases/tomdroid-notes.db
        int rwId = 0;
        thread_id_t tid = 0;
        address_t obj = 0;
        char name[128] = {0,};
        char field[128] = {0,};

        if(sscanf(line,"rwId:%d WRITE tid:%d obj:%X class:%s field:%s",&rwId,&tid,&obj,name,field)!=5){
        if(sscanf(line,"wId:%d WRITE tid:%d obj:%X class:%s field:%s",&rwId,&tid,&obj,name,field)!=5){
        if(sscanf(line,"MrwId:%d WRITE tid:%d obj:%X class:%s field:%s",&rwId,&tid,&obj,name,field)!=5){
        if(sscanf(line,"MwId:%d WRITE tid:%d obj:%X class:%s field:%s",&rwId,&tid,&obj,name,field)!=5){
          if(sscanf(line,"rwId:%d WRITE tid:%d database:%s",&rwId,&tid,name)!=3){
          if(sscanf(line,"wId:%d WRITE tid:%d database:%s",&rwId,&tid,name)!=3){
          if(sscanf(line,"MrwId:%d WRITE tid:%d database:%s",&rwId,&tid,name)!=3){
          if(sscanf(line,"MwId:%d WRITE tid:%d database:%s",&rwId,&tid,name)!=3){
            printf("[ERROR] WRITE parsing fails - %s\n",line);
            return -1;
          }}}}
        }}}}

	/*item =  new WorkItem(WRITE, tid, -1,-1,-1,-1,-1,rwId,obj,name, field);
	q->add(item);*/
//Salman
        mem_write(tid,rwId,obj,name,field);
	LOGD("RCVD WRITE \n");
      }
      else if(strcmp(p,"WRITE-STATIC")==0){
        //rwId:570 WRITE-STATIC tid:1 class:Lorg/tomdroid/util/Preferences; field:321
        int rwId = 0;
        thread_id_t tid = 0;
        address_t obj = 0;
        char name[128] = {0,};
        char field[128] = {0,};

        if(sscanf(line,"rwId:%d WRITE-STATIC tid:%d class:%s field:%s",&rwId,&tid,name,field)!=4){
        if(sscanf(line,"wId:%d WRITE-STATIC tid:%d class:%s field:%s",&rwId,&tid,name,field)!=4){
        if(sscanf(line,"MrwId:%d WRITE-STATIC tid:%d class:%s field:%s",&rwId,&tid,name,field)!=4){
        if(sscanf(line,"MrwId:%d WRITE-STATIC tid:%d class:%s field:%s",&rwId,&tid,name,field)!=4){
          printf("[ERROR] WRITE-STATIC parsing fails - %s\n",line);
          return -1;
        }}}}

	/*item =  new WorkItem(WRITESTATIC, tid, -1,-1,-1,-1,-1,rwId,obj,name, field);
	q->add(item);*/
//Salman
        mem_write(tid,rwId,obj,name,field);
	LOGD("RCVD WRITE");
      }
    }
	sleep(1);
  }

  droidtrack_finish();

  sleep(1);
    printf("Enter Ctrl-C to end the program...\n");
    while (1);
    exit(0);
  //return 0;
}
