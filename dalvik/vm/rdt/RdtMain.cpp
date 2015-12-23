/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * RDT initialization.
 */
#include "Dalvik.h"
#include "Atomic.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>


static void* rdtThreadStart(void* arg);

list<WorkItem*> queue;
void wqueueInit() {
        gDvm.q = new wqueue<WorkItem*>(queue);
}

/*
 * Initialize RDT.
 *
 */
int dvmRdtStartup()
{
    pthread_t rdtThreadHandle;
    //init wqueue
    wqueueInit();
     //init droidtrack
    droidtrack_init();
    if (!dvmCreateInternalThread(&rdtThreadHandle, "RDT",
            rdtThreadStart, NULL))
    {
        goto fail;
    }
    LOGD("THREAD CREATED SUCCESSFULLY======");
    return 1;

fail:
    return 0;
}

/*
 * Entry point for RDT thread.  The thread was created through the VM
 * mechanisms, so there is a java/lang/Thread associated with us.
 */
static void* rdtThreadStart(void* arg)
{
	Thread* self = dvmThreadSelf();
	int first;
     	//dvmDbgThreadWaiting();
        while (1) {
		dvmChangeStatus(self, THREAD_VMWAIT);
                WorkItem* item = gDvm.q->remove();
        	dvmChangeStatus(self, THREAD_RUNNING);
		switch(item->event_name) {
                   case UNLOCK:
                        unlock(item->thread_id,item->lock_addr);
			LOGD("RCVD UNLOCK \n");
                   break;
                   case LOCK:
                        droid_lock(item->thread_id,item->lock_addr); //working
			LOGD("RCVD LOCK \n");
                   break;
                   case FORK:
                        thread_create(item->thread_id,item->child_thread_id); //working
			LOGD("RCVD FORK \n");
                   break;
                   case THREADINIT:
                        thread_init(item->thread_id, 0);   //working
			LOGD("RCVD THREADINIT \n");
                   break;
                   case THREADEXIT:
                        thread_exit(item->thread_id);
			LOGD("RCVD THREADEXIT \n");  //working
                   break;
                   case JOIN:
                        thread_join(item->thread_id,item->child_thread_id);
			LOGD("RCVD JOIN \n");  //working (no called though)
                   break;
                   case LOOP:
                        looponq(item->thread_id);	//working
			LOGD("RCVD LOOP \n");
                   break;
                   case ATTACHQ:
                        attachq(item->thread_id);  //working
			LOGD("RCVD ATTACHQ \n");
                   break;
                   case ENABLELIFECYCLE:
                        enable(item->thread_id,NA);  //working
			LOGD("RCVD ENABLELIFECYCLE \n");
                   break;
                   case ENABLEEVENT:
                        enable(item->thread_id,NA);   //working (not called though)
			LOGD("RCVD ENABLEEVENT \n");
                   break;
                   case POST:
			LOGD("RCVD POST \n");  //working
			if(item->delay == -1) {
			  first = 1;
			  item->delay = 0;
			}
			else {
			  first = 0;
			}
                        post (item->thread_id, NA, item->tsk_id, item->dest_thread_id, item->delay, first);
                   break;
                   case CALLMETHOD:
                        task_begin(item->thread_id,item->tsk_id); //working
			LOGD("RCVD CALLMETHOD \n");
                   break;
                   case RET:
                        task_end(item->thread_id,item->tsk_id); //working
			LOGD("RCVD RET \n");
                   break;
                   case READ:
                        mem_read(item->thread_id,item->rwId,item->obj,item->class_name,item->field_name); //working
			LOGD("RCVD READ \n");
                   break;
                   case READSTATIC:
                        mem_read(item->thread_id,item->rwId,item->obj,item->class_name,item->field_name); //working
			LOGD("RCVD READSTATIC \n");
                   break;
                   case WRITE:
                        mem_write(item->thread_id,item->rwId,item->obj,item->class_name,item->field_name); //working
			LOGD("RCVD WRITE \n");
		   break;
                   case WRITESTATIC:
                        mem_write(item->thread_id,item->rwId,item->obj,item->class_name,item->field_name); //working
			LOGD("RCVD WRITESTATIC \n");
                   break;
                }
                //LOGD("removed item: message - %s, number - %d\n",
                //	item->getMessage(), item->getNumber());
                delete item;
		LOGD("queue size after remove %d\n", gDvm.q->size());
        }
    return NULL;
}
