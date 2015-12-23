/*
 * Copyright (C) 2013, Indian Institute of Science
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

/* This is a native library to model check Android apps to detect
 * concurrency bugs.
 *
 * @author Pallavi Maiya
 */

/*todo:
 *
 */

#include "abc.h"
#include "Dalvik.h"
#define SALMAN_CODE
struct AbcGlobals* gAbc;

std::map<const char*, AbcClassField*> abcClassToFieldsMap;

// This maps the threadid to its corresponding stackentries.
// Lifetime: During Trace generation. Should be completely freed by 
// analysis time.
std::map<int, AbcMethodStack*> abcThreadStackMap;

// This maps the threadid to its corresponding entry app method in stack.
// Lifetime: During trace generation. Should be completely freed by 
// analysis time.
std::map<int, AbcMethod*> abcThreadBaseMethodMap;

// required during trace-generation
std::map<int, AbcCurAsync*> abcThreadCurAsyncMap;

// maps a thread-id to caller objects.
// needed only during trace generation.
// This is required to track java library calls.
std::map<int, AbcMethObj*> abcLibCallerObjectMap;

// Required only during trace generation.
// There will be as many as the number of unique messages.
std::map<u4, AbcMsg*> abcUniqueMsgMap;



// Cleanup  this strcuture completely.
std::map<int, AbcThreadIds*> abcLogicalThreadIdMap;

// Cleanup  this strcuture completely.
std::map<std::string, AbcDbAccess*> abcDbAccessMap;

// Cleanup  this strcuture completely.
//std::set<int> abcThreadIdSet;



// maps a thread-id to some meta-data related to the thread.
// Lifetime: Required during analysis.
// Remove all the fields in this except the first two.
// Not many objects in an app. Just as many as the number of createc
// threads.
std::map<int, AbcThread*> abcThreadMap;

// maps a global integer number generation to the sequence of
// operations. This is the main program trace. This is required
// throughout the program analysis. 
std::map<int, AbcOp*> abcTrace;

// Required throughout analysis
// There will be as many as the number of messages.
std::map<int, AbcAsync*> abcAsyncMap;

// Required throughout analysis
// There will be as many as the number of read-writes.
std::map<int, AbcRWAccess*> abcRWAccesses;

// Required throughout analysis
// There will be as many as the number of read-write sets.
// Each one inturn points to a list of integers which indicate a read-write access index
// generated in abcRWAccesses.
std::map<int, std::pair<int, std::list<int> > > abcRWAbstractionMap;


// required throughout
std::map<std::pair<Object*, u4>, std::pair<std::set<int>, std::set<int> > > abcObjectAccessMap;
std::map<std::string, std::pair<std::set<int>, std::set<int> > > abcDatabaseAccessMap;
std::map<std::pair<const char*, u4>, std::pair<std::set<int>, std::set<int> > > abcStaticAccessMap;

//required during analysis
std::map<int, int> notifyWaitMap;

// required during analysis
std::map<int, AbcThreadBookKeep*> abcThreadBookKeepMap;

//needed only during trace generation to track async-blocks to be deleted.
//clean this up before graph closure
std::map<int, std::pair<bool,bool> > abcAsyncStateMap;

//tracks next access set id for read-write operations performed by thread.
//needed only during trace generation 
std::map<int, int> abcThreadAccessSetMap;

//tracks lock-unlock performed on an object. 
//gets filled during analysis stage
std::map<Object*, AbcLock*> abcLockMap;

//used to track nested synchronize locks on same object.
//needed only during trace generation
std::map<Object*, AbcLockCount*> abcLockCountMap;

//needed only during trace generation
std::map<u4, std::set<int> > abcViewEventMap;

//maps from events to enableOpId, triggerOpId
//filled and used during analysis stage
std::map<std::pair<u4, int>, std::pair<int,int> > abcEnabledEventMap;
//filled and used during analysis stage
std::map<std::pair<int, int>, std::pair<int,int> > abcEnabledLifecycleMap;
//filled during analysis stage
std::map<std::string, std::pair<int,int> > abcRegisteredReceiversMap;

std::map<u4, std::pair<int, int> > abcWindowFocusChangeMap;

std::map<std::pair<u4, int>, std::pair<int, AbcAsync*> > abcIdleHandlerMap; 

//needed only during trace generation
std::map<int, AbcReceiver*> abcDelayedReceiverTriggerThreadMap;
std::map<int, AbcReceiver*> abcDelayedReceiverTriggerMsgMap;
//a map from asyncId to its corresponding enable event id if the async block is due to it
std::map<int, int> abcAsyncEnableMap;


//sets needed to collect race related statistics
//for objects / static classes
std::set<std::pair<const char*, u4> > multiThreadRaces;
std::set<std::pair<const char*, u4> > delayPostRaces;
std::set<std::pair<const char*, u4> > crossPostRaces;
//maintain info on the co-enabled events i.e events whose triggers have no HB relation
std::map<std::pair<const char*, u4>, std::pair<int,int> > coEnabledEventUiRaces;
std::map<std::pair<const char*, u4>, std::pair<int,int> > coEnabledEventNonUiRaces; 

std::set<std::pair<const char*, u4> > fieldSet;
std::set<std::pair<const char*, u4> > raceyFieldSet; //keeps count of disinct async races

std::set<std::pair<const char*, u4> > uncategorizedRaces;

//taking count of races with object,field sensitivity wherever applicable
std::set<std::pair<Object*, u4> > multiThreadRacesObj;
std::set<std::pair<Object*, u4> > delayPostRacesObj;
std::set<std::pair<Object*, u4> > crossPostRacesObj;
//maintain info on the co-enabled events i.e events whose triggers have no HB relation
std::map<std::pair<Object*, u4>, std::pair<int,int> > coEnabledEventUiRacesObj;
std::map<std::pair<Object*, u4>, std::pair<int,int> > coEnabledEventNonUiRacesObj;

std::set<std::pair<Object*, u4> > uncategorizedRacesObj;

//taking count of races with class,field sensitivity wherever applicable
std::set<std::pair<const char*, u4> > multiThreadRacesClass;
std::set<std::pair<const char*, u4> > delayPostRacesClass;
std::set<std::pair<const char*, u4> > crossPostRacesClass;
//maintain info on the co-enabled events i.e events whose triggers have no HB relation
std::map<std::pair<const char*, u4>, std::pair<int,int> > coEnabledEventUiRacesClass;
std::map<std::pair<const char*, u4>, std::pair<int,int> > coEnabledEventNonUiRacesClass;

std::set<std::pair<const char*, u4> > uncategorizedRacesClass;


//for databases
std::set<std::string> dbMultiThreadRaces;
std::set<std::string> dbDelayPostRaces;
std::set<std::string> dbCrossPostRaces;
std::map<std::string, std::pair<int,int> > dbCoEnabledEventRaces;

std::set<std::string> dbFieldSet;
std::set<std::string> dbRaceyFieldSet; //keeps count of distict async races

std::set<std::string> dbUncategorizedRaces;

//initialize during startup
std::set<std::string> UiWidgetSet;

int abcThreadCount = 1;
int abcMsgCount = 20;
int abcOpCount = 1;
int abcAccessSetCount = 1;
int abcRWCount = 1;
int abcEventCount = 0;
int abcTraceLengthLimit = -1;

int abcStartOpId;
bool ** adjGraph;
WorklistElem* worklist = NULL; //a list 
std::map<int, std::pair<Destination*, Source*> > adjMap; 

//some fields for race detection related statistics collection
int abcForkCount = 1; 
int abcMQCount = 0; //since attach-q of main thread happens after ABC starts it gets logged
int abcEventTriggerCount = 0;


void cleanupBeforeExit(){
    //free abcThreadMap
    if(abcThreadMap.size() > 0){
        std::map<int, AbcThread*>::iterator delIter = abcThreadMap.begin();
        while(delIter != abcThreadMap.end()){
            AbcThread* ptr = delIter->second;
            abcThreadMap.erase(delIter++);
            free(ptr);
        }    
    }

    //free abstraction map
    if(abcRWAbstractionMap.size() > 0){
        std::map<int, std::pair<int, std::list<int> > >::iterator delIter = abcRWAbstractionMap.begin();
        while(delIter != abcRWAbstractionMap.end()){
            delIter->second.second.clear();
            abcRWAbstractionMap.erase(delIter++);
        }
    }

    //free object-database-static access map
    if(abcObjectAccessMap.size() > 0){
        std::map<std::pair<Object*, u4>, std::pair<std::set<int>, std::set<int> > >::iterator delIter = abcObjectAccessMap.begin();
        while(delIter != abcObjectAccessMap.end()){
            delIter->second.first.clear();
            delIter->second.second.clear();
            abcObjectAccessMap.erase(delIter++);
        }
    }

    if(abcDatabaseAccessMap.size() > 0){
        std::map<std::string, std::pair<std::set<int>, std::set<int> > >::iterator delIter = abcDatabaseAccessMap.begin();
        while(delIter != abcDatabaseAccessMap.end()){
            delIter->second.first.clear();
            delIter->second.second.clear();
            abcDatabaseAccessMap.erase(delIter++);
        }
    }

    if(abcStaticAccessMap.size() > 0){
        std::map<std::pair<const char*, u4>, std::pair<std::set<int>, std::set<int> > >::iterator delIter = abcStaticAccessMap.begin();
        while(delIter != abcStaticAccessMap.end()){
            delIter->second.first.clear();
            delIter->second.second.clear();
            abcStaticAccessMap.erase(delIter++);
        }
    }

    //free read writes
    if(abcRWAccesses.size() > 0){
        std::map<int, AbcRWAccess*>::iterator delIter = abcRWAccesses.begin();
        while(delIter != abcRWAccesses.end()){
            AbcRWAccess* ptr = delIter->second;
            abcRWAccesses.erase(delIter++);
            free(ptr->dbPath);
            free(ptr->field);
            free(ptr);
        }
    }

    //free abcThreadBookKeepMap 
    if(abcThreadBookKeepMap.size() > 0){
        std::map<int, AbcThreadBookKeep*>::iterator delIter = abcThreadBookKeepMap.begin();
        while(delIter != abcThreadBookKeepMap.end()){
            AbcThreadBookKeep* ptr = delIter->second;
            abcThreadBookKeepMap.erase(delIter++);
            free(ptr);
        }
    }

    //free abcLockMap
    if(abcLockMap.size() > 0){
        std::map<Object*, AbcLock*>::iterator delIter = abcLockMap.begin();
        while(delIter != abcLockMap.end()){
            AbcLock* ptr = delIter->second;
            abcLockMap.erase(delIter++);

            std::map<int, UnlockList*>::iterator iter = ptr->unlockMap.begin();
            while(iter != ptr->unlockMap.end()){
                UnlockList* lockPtr = iter->second;
                ptr->unlockMap.erase(iter++);
                free(lockPtr);
            }            
            
            free(ptr);
        }
    }    

    //clearing async map
    if(abcAsyncMap.size() > 0){
        std::map<int, AbcAsync*>::iterator delIter = abcAsyncMap.begin();
        while(delIter != abcAsyncMap.end()){
            AbcAsync* ptr = delIter->second;
            abcAsyncMap.erase(delIter++);
            free(ptr);
        }
    }

    //clearing abcTrace
    if(abcTrace.size() > 0){
        std::map<int, AbcOp*>::iterator delIter = abcTrace.begin();
        while(delIter != abcTrace.end()){
            AbcOp* ptr = delIter->second;
            abcTrace.erase(delIter++);
            free(ptr->arg2);
            free(ptr->arg5);
            free(ptr);
        }
    }

    //cleanup datarace stats collection related things
    multiThreadRaces.clear();
    delayPostRaces.clear();
    crossPostRaces.clear();
    coEnabledEventUiRaces.clear();
    coEnabledEventNonUiRaces.clear();
    fieldSet.clear();
    raceyFieldSet.clear();
    uncategorizedRaces.clear();
    dbMultiThreadRaces.clear();
    dbDelayPostRaces.clear();
    dbCrossPostRaces.clear();
    dbCoEnabledEventRaces.clear();
    dbFieldSet.clear();
    dbRaceyFieldSet.clear();
    dbUncategorizedRaces.clear();
 
}

bool abcIsClassInClassToFieldsMap(const char* clazz){
    std::map<const char*, AbcClassField*>::iterator it = abcClassToFieldsMap.find(clazz);
    if(it == abcClassToFieldsMap.end()){
        return false;
    }else{
        return true;
    }
}

AbcClassField* abcGetClassIfInClassFieldsMap(const char* clazz){
    std::map<const char*, AbcClassField*>::iterator it = abcClassToFieldsMap.find(clazz);
    if(it != abcClassToFieldsMap.end()){
        return it->second;
    }else{
        return NULL;
    }
}

AbcClassField* abcAddAndGetClassToClassFieldsMap(const char* clazz, int firstFieldOffset){
    AbcClassField* abcClazz = new AbcClassField;
    abcClazz->firstFieldOffset = firstFieldOffset;
    if(abcClazz != NULL){
        abcClassToFieldsMap.insert(std::make_pair(clazz,abcClazz));
    }
    return abcClazz;
}

void abcStoreFieldNameForClass(AbcClassField* clazz, const char* fieldName, int fieldOffset){
    std::map<int, std::string>::iterator it = clazz->fieldOffsetToNameMap.find(fieldOffset); 
    if(it == clazz->fieldOffsetToNameMap.end()){
        std::string fieldStr(fieldName);
        clazz->fieldOffsetToNameMap.insert(std::make_pair(fieldOffset,fieldStr));
    }
}

std::string abcGetFieldNameOfClassFieldId(Object* obj, int fieldId){
    std::string fieldName("");
    const char* tmpClassName = obj->clazz->descriptor;
    ClassObject* tmpClassObj = obj->clazz;
    bool foundField = false;
    
    while(!foundField){
      std::map<const char*, AbcClassField*>::iterator it = abcClassToFieldsMap.find(tmpClassName);
      if(it != abcClassToFieldsMap.end()){
          AbcClassField* abcClazz = it->second;
          if(abcClazz->firstFieldOffset <= fieldId){
              std::map<int, std::string>::iterator it2 = abcClazz->fieldOffsetToNameMap.find(fieldId);
              if(it2 != abcClazz->fieldOffsetToNameMap.end()){
                  fieldName.assign(it2->second); 
                  foundField = true;
              }else{
                  LOGE("ABC-MISSING: Could not locate field id %d in class %s", fieldId, tmpClassName);
                  foundField = true;
              }
          }else{
              if(tmpClassObj->super != NULL){
                  tmpClassName = tmpClassObj->super->descriptor;
                  tmpClassObj = tmpClassObj->super;
              }else{
                  LOGE("ABC-MISSING: Could not locate class super class of %s in our internal ClassFieldMap", tmpClassName);
                  foundField = true;
              }
          }
      }else{
          LOGE("ABC-MISSING: Could not locate class %s in our internal ClassFieldMap", tmpClassName);
          foundField = true;
      } 
    }

    return fieldName;
}

void abcAddLockOpToTrace(Thread* self, Object* obj, int lock){
    /*lock/unlock should be tracked even when shouldABCTrack = false
     *and a app method based thread stack exists for a thread. 
     *In such tracking when shouldABCTrack = false, we only record
     *those lock/unlocks which have any non-lock or non-unlock operation
     *in between.
     */
    if(self->shouldABCTrack == true){
    //    if(abcThreadCurAsyncMap.find(self->threadId)->second->shouldRemove == false){
            abcLockMutex(self, &gAbc->abcMainMutex);
            if(gDvm.isRunABC == true){
                bool addToTrace = checkAndIncrementLockCount(obj, self->abcThreadId);
                if(addToTrace){
                    AbcCurAsync* curAsync = abcThreadCurAsyncMap.find(self->abcThreadId)->second;
                    if(curAsync->shouldRemove == false && (!curAsync->hasMQ || curAsync->asyncId != -1)){
                        LOGD("Lock is from ABC and value is %d and lock value is %d",obj->lock,lock);
                        addLockToTrace(abcOpCount++, self->abcThreadId, obj);
                    }else{
                        LOGE("ABC-DONT-LOG: found a lock operation in deleted async block. not logging it");
                    }
                }
            }
            abcUnlockMutex(&gAbc->abcMainMutex);
    /*    }else{
            LOGE("Trace has a LOCK for a async block forced to be deleted which is not addressed by "
                " implementation. Cannot continue further");
            gDvm.isRunABC = false;
            return;
        } */

    }else if(abcLibCallerObjectMap.find(self->threadId)
                    != abcLibCallerObjectMap.end()){
        if(isObjectInThreadAccessMap(self->threadId,obj)){
            abcLockMutex(self, &gAbc->abcMainMutex);
            if(gDvm.isRunABC == true){
                bool addToTrace = checkAndIncrementLockCount(obj, self->abcThreadId);
                if(addToTrace){
                    AbcCurAsync* curAsync = abcThreadCurAsyncMap.find(self->abcThreadId)->second;
                    if(curAsync->shouldRemove == false && (!curAsync->hasMQ || curAsync->asyncId != -1)){
                      LOGD("Lock is not from ABC and lock value is %d",lock);  
                      addLockToTrace(abcOpCount++, self->abcThreadId, obj);
                    }else{
                        LOGE("ABC-DONT-LOG: found a lock operation in deleted async block. not logging it");
                    }
                }
            }
            abcUnlockMutex(&gAbc->abcMainMutex);
        }
    }
}

void abcAddUnlockOpToTrace(Thread* self, Object* obj, int lock){
     //LOGD("Unlock Check value is %d",obj->lock);
     if(self->shouldABCTrack == true){
            abcLockMutex(self, &gAbc->abcMainMutex);
            if(gDvm.isRunABC == true){
                bool addToTrace = checkAndDecrementLockCount(obj, self->abcThreadId);
                if(addToTrace){
                    AbcCurAsync* curAsync = abcThreadCurAsyncMap.find(self->abcThreadId)->second;
                    if(curAsync->shouldRemove == false && (!curAsync->hasMQ || curAsync->asyncId != -1)){
                       LOGD("Lock is from ABC and value is %d and lock is %d",obj->lock,lock); 
                       addUnlockToTrace(abcOpCount++, self->abcThreadId, obj);
                    }else{
                        LOGE("ABC-DONT-LOG: found a unlock operation in deleted async block. not logging it");
                    }
                }
            }
            abcUnlockMutex(&gAbc->abcMainMutex);

    }else if(abcLibCallerObjectMap.find(self->threadId)
                    != abcLibCallerObjectMap.end()){
        if(isObjectInThreadAccessMap(self->threadId,obj)){
            abcLockMutex(self, &gAbc->abcMainMutex);
            if(gDvm.isRunABC == true){
                bool addToTrace = checkAndDecrementLockCount(obj, self->abcThreadId);
                if(addToTrace){
                    AbcCurAsync* curAsync = abcThreadCurAsyncMap.find(self->abcThreadId)->second;
                    if(curAsync->shouldRemove == false && (!curAsync->hasMQ || curAsync->asyncId != -1)){
                       LOGD("Unlock is not from ABC and value is %d",lock); 
                       addUnlockToTrace(abcOpCount++, self->abcThreadId, obj);
                    }else{
                        LOGE("ABC-DONT-LOG: found a unlock operation in deleted async block. not logging it");
                    }
                }
            }
            abcUnlockMutex(&gAbc->abcMainMutex);
        }
    }
}

void abcAddCallerObjectForLibMethod(int abcTid, const Method* meth, Object* obj){
    if(strcmp(meth->name,"<clinit>") != 0 && strcmp(meth->name,"<init>") != 0 && 
          strcmp(meth->clazz->descriptor,
                "Ljava/lang/ClassLoader;") != 0 && strcmp(meth->clazz->descriptor,
                    "Ldalvik/system/PathClassLoader;") != 0 && strcmp(
                     meth->clazz->descriptor,"Ljava/lang/BootClassLoader") != 0){
        std::map<int,AbcMethObj*>::iterator it = abcLibCallerObjectMap.find(abcTid);
        AbcMethObj* mo = (AbcMethObj*)malloc(sizeof(AbcMethObj));
        mo->method = meth;
        mo->obj = obj;
        mo->prev = NULL;
        if(it != abcLibCallerObjectMap.end()){
            mo->prev = it->second;
            it->second = mo;
        }else{
            abcLibCallerObjectMap.insert(std::make_pair(abcTid,mo));
        }
   }
}

void abcRemoveCallerObjectForLibMethod(int abcTid, const Method* meth){
    std::map<int,AbcMethObj*>::iterator it = abcLibCallerObjectMap.find(abcTid);
    if(strcmp(meth->name,"<clinit>") != 0 && strcmp(meth->name,"<init>") != 0 &&
          strcmp(meth->clazz->descriptor,
                "Ljava/lang/ClassLoader;") != 0 && strcmp(meth->clazz->descriptor,
                    "Ldalvik/system/PathClassLoader;") != 0 && strcmp(
                     meth->clazz->descriptor,"Ljava/lang/BootClassLoader") != 0 && 
                     it != abcLibCallerObjectMap.end()){
        if(it->second != NULL && it->second->method == meth){ 
            if(it->second->prev != NULL){
                AbcMethObj* tmpPtr = it->second;
                it->second = it->second->prev;
                free(tmpPtr);
            }else{
                AbcMethObj* tmpPtr = it->second;
                abcLibCallerObjectMap.erase(abcTid);
                free(tmpPtr);
            }
        }else{
         //   LOGE("ABC-MISSING: caller object for method %s has been suspiciously removed for thread %d", meth->name, abcTid);
            abcLibCallerObjectMap.erase(abcTid);
        }
    }
}

bool isObjectInThreadAccessMap(int abcThreadId, Object* obj){
    std::map<int, AbcMethObj*>::iterator it;
    bool retVal = false;
    AbcMethObj* tmp;
    if((it = abcLibCallerObjectMap.find(abcThreadId)) != abcLibCallerObjectMap.end()){
        tmp = it->second;
        while(tmp != NULL){
            if(tmp->obj == obj){
                retVal = true;
                break;
            }else{
                tmp = tmp->prev;
            }                
        }
    }
    return retVal;
}

int abcGetRecursiveLockCount(Object* lockObj){
    std::map<Object*, AbcLockCount*>::iterator it = abcLockCountMap.find(lockObj);
    if(it == abcLockCountMap.end()){
        return 0;
    }else{
        return it->second->count;
    }
}

bool checkAndIncrementLockCount(Object* lockObj, int threadId){
    bool addToTrace = false;
    std::map<Object*, AbcLockCount*>::iterator it = abcLockCountMap.find(lockObj);
    if(it == abcLockCountMap.end()){
        AbcLockCount* alc = (AbcLockCount*)malloc(sizeof(AbcLockCount));
        alc->threadId = threadId;
        alc->count = 1;
        abcLockCountMap.insert(std::make_pair(lockObj, alc));
        addToTrace = true;
    }else{
        if(it->second->threadId == threadId){
            it->second->count++;
        }else{
            LOGE("ABC: A thread %d is taking a lock %p when another thread %d holds the lock."
                 " The trace seems to have missed an unlock. Aborting trace generation.", 
                 threadId, lockObj, it->second->threadId);
            gDvm.isRunABC = false;
        }
    }
    return addToTrace;
}

bool checkAndDecrementLockCount(Object* lockObj, int threadId){
    bool addToTrace = false;
    std::map<Object*, AbcLockCount*>::iterator it = abcLockCountMap.find(lockObj);
    if(it == abcLockCountMap.end()){
        LOGE("ABC: Unlock seen by trace without a prior lock. We might have missed a "
             "lock statement. Aborting trace generation. tid:%d  lock: %p", threadId, lockObj);
        gDvm.isRunABC = false;
    }else{
        if(it->second->threadId == threadId){
            it->second->count--;
            if(it->second->count == 0){
                addToTrace = true;
                abcLockCountMap.erase(lockObj);
            }
        }else{
            LOGE("ABC: A thread %d is performing an unlock when another thread holds the lock %p."
                 " The trace seems to have missed an unlock / lock. Aborting trace generation.", threadId, lockObj);
            gDvm.isRunABC = false;
        }
    }
    return addToTrace;
}

bool isThreadStackEmpty(int abcThreadId){
    bool retVal = true;
    std::map<int, AbcMethodStack*>::iterator it;
    if((it = abcThreadStackMap.find(abcThreadId)) !=  abcThreadStackMap.end()){
        if(it->second != NULL)
            retVal = false;
    }
    return retVal;
}

void abcPushMethodForThread(int threadId, const Method* method){
    std::map<int, AbcMethodStack*>::iterator it;
    AbcMethodStack* mStack = (AbcMethodStack *)malloc(sizeof(AbcMethodStack));
    mStack->method = method;
    mStack->prev = NULL;
    if((it = abcThreadStackMap.find(threadId)) !=  abcThreadStackMap.end()){
        mStack->prev = it->second;
        it->second = mStack;
    }else{
        abcThreadStackMap.insert(std::make_pair(threadId, mStack));
    }
}

void abcStoreBaseMethodForThread(int threadId, const Method* method) {
	if(abcThreadBaseMethodMap.find(threadId) == abcThreadBaseMethodMap.end()){
            /*allocate the struct on heap so that its accessible even after 
             *method exit */
            AbcMethod* am = (AbcMethod *)malloc(sizeof(AbcMethod));
            am->method = method;
            abcThreadBaseMethodMap.insert(std::make_pair(threadId, am));
        }else{
            LOGE("ABC: there is already a base method associated with the thread.");
        } 
}

/*returns NULL if no base-app-method for the specified thread id*/
const Method* abcGetBaseMethodForThread(int threadId){
    std::map<int, AbcMethod*>::iterator it;
    if((it = abcThreadBaseMethodMap.find(threadId)) == abcThreadBaseMethodMap.end()){
        return NULL;
    }else{
        return abcThreadBaseMethodMap.find(threadId)->second->method;
    }
}

void abcRemoveBaseMethodForThread(int threadId){
    std::map<int, AbcMethod*>::iterator it;
    if((it = abcThreadBaseMethodMap.find(threadId)) != abcThreadBaseMethodMap.end()){
        AbcMethod* tmpPtr = it->second;
        abcThreadBaseMethodMap.erase(it->first);
        free(tmpPtr);
    }else{
        LOGE("ABC: did not find the base method to be remove!");
    }
}

void abcRemoveThreadFromStackMap(int threadId){
    std::map<int, AbcMethodStack*>::iterator it;
    if((it = abcThreadStackMap.find(threadId)) !=  abcThreadStackMap.end()){
        AbcMethodStack* tmpPtr = it->second; 
        abcThreadStackMap.erase(it->first);
        free(tmpPtr);
    }
}

const Method* abcGetLastMethodInThreadStack(int threadId){
    std::map<int, AbcMethodStack*>::iterator it;
    if((it = abcThreadStackMap.find(threadId)) !=  abcThreadStackMap.end()){
        if(it->second == NULL)
            return NULL;
        else
            return it->second->method;
    }else{
        return NULL;
    }
}

/*erases the last method entry in stack and returns the method*/
const Method* abcPopLastMethodInThreadStack(int threadId){
    std::map<int, AbcMethodStack*>::iterator it;
    if((it = abcThreadStackMap.find(threadId)) !=  abcThreadStackMap.end()){
       const Method* meth = it->second->method;
       if(it->second->prev == NULL){
           AbcMethodStack* tmpPtr = it->second;
           abcThreadStackMap.erase(it->first);
           free(tmpPtr);
       }else{
           AbcMethodStack* tmpPtr = it->second;
           it->second = it->second->prev;
           free(tmpPtr); 
       }
       return meth;
    }else{
        return NULL;
    }
}

void abcAddThreadToMap(Thread* self, const char* name){
    AbcThread* abcThread =  (AbcThread *)malloc(sizeof(AbcThread));
    abcThread->threadId = self->threadId;
    abcThread->isOriginUntracked = false;

    /*initialize the conditional variable associated by ABC with a thread
    pthread_cond_init(&self->abcCond, NULL);
    abcThread->abcCond = &self->abcCond; */
    
    abcThreadMap.insert(std::make_pair(self->abcThreadId, abcThread));
}

void addThreadToCurAsyncMap(int threadId){
    AbcCurAsync* async = (AbcCurAsync*)malloc(sizeof(AbcCurAsync));
    async->asyncId = -1;
    async->shouldRemove = false;
    async->hasMQ = false;
    abcThreadCurAsyncMap.insert(std::make_pair(threadId, async));
}

void abcAddLogicalIdToMap(int threadId, int abcThreadId){
    std::map<int, AbcThreadIds*>::iterator it = abcLogicalThreadIdMap.find(threadId);  
    AbcThreadIds* ati = (AbcThreadIds *)malloc(sizeof(AbcThreadIds));
    ati->abcThreadId = abcThreadId;
    if(it != abcLogicalThreadIdMap.end()){
        ati->prevAbcId = it->second;
        it->second = ati;
    }else{
        ati->prevAbcId = NULL;
        abcLogicalThreadIdMap.insert(std::make_pair(threadId, ati));
    }
}

int abcGetAbcThreadId(int threadId){
    int abcId = -1;
    std::map<int, AbcThreadIds*>::iterator it = abcLogicalThreadIdMap.find(threadId);
    if(it != abcLogicalThreadIdMap.end()){
        abcId = it->second->abcThreadId;
    }
    return abcId;
}

void abcRemoveThreadFromLogicalIdMap(int threadId){
    std::map<int, AbcThreadIds*>::iterator it = abcLogicalThreadIdMap.find(threadId);
    if(it != abcLogicalThreadIdMap.end()){
        abcLogicalThreadIdMap.erase(it);
    }
}

bool abcIsThreadOriginUntracked(int abcThreadId){
    bool isOriginUntracked = true;
    std::map<int, AbcThread*>::iterator it = abcThreadMap.find(abcThreadId);
    if(it != abcThreadMap.end()){
        isOriginUntracked = it->second->isOriginUntracked;
    }
    return isOriginUntracked;
}

void abcAddDbAccessInfo(std::string dbPath, int trId, int accessType, int abcThreadId){
    
    std::map<std::string, AbcDbAccess*>::iterator it = abcDbAccessMap.find(dbPath);
    if(it == abcDbAccessMap.end()){
        AbcDbAccess* ada = (AbcDbAccess *)malloc(sizeof(AbcDbAccess));

        ada->dbAccessForThread = (AbcDbAccessType *)malloc(sizeof(AbcDbAccessType));
        ada->dbAccessForThread->accessType = accessType;
        ada->dbAccessForThread->transitionId = trId;
        ada->dbAccessForThread->prevAccess = NULL;
        
        ada->abcThreadId = abcThreadId;
        ada->nextThreadDb = NULL;
                
        abcDbAccessMap.insert(std::make_pair(dbPath, ada));
    }else{
        AbcDbAccess *tmpDba = it->second;
        while(tmpDba != NULL){
            if(tmpDba->abcThreadId == abcThreadId){
                AbcDbAccessType *adt = (AbcDbAccessType *)malloc(sizeof(AbcDbAccessType));
                adt->accessType = accessType;
                adt->transitionId = trId;
                adt->prevAccess = tmpDba->dbAccessForThread;
                tmpDba->dbAccessForThread = adt;
                
                break;
            }
            if(tmpDba->nextThreadDb == NULL){
                AbcDbAccess* ada = (AbcDbAccess *)malloc(sizeof(AbcDbAccess));
                ada->dbAccessForThread = (AbcDbAccessType *)malloc(sizeof(AbcDbAccessType));
                ada->dbAccessForThread->accessType = accessType;
                ada->dbAccessForThread->transitionId = trId;
                ada->dbAccessForThread->prevAccess = NULL;

                ada->abcThreadId = abcThreadId;
                ada->nextThreadDb = NULL;
                tmpDba->nextThreadDb = ada;
                break;
            }            
            tmpDba = tmpDba->nextThreadDb;
        }
    }
}

/*print stack trace associated with a thread*/
void abcPrintThreadStack(int threadId){
    std::map<int, AbcMethodStack*>::iterator it;
    if((it = abcThreadStackMap.find(threadId)) != abcThreadStackMap.end()){
        AbcMethodStack* ams = it->second;
        while(ams != NULL){
            LOGE("ABC: method in stack - %s", ams->method->name);
            ams = ams->prev; 
        }
     }
}

void abcLockMutex(Thread* self, pthread_mutex_t* pMutex){   

    ThreadStatus oldStatus;
    
    if (self == NULL)       // try to get it from TLS 
        self = dvmThreadSelf();

    if (self != NULL) {
        oldStatus = self->status;
        self->status = THREAD_VMWAIT;
    } else {
        // happens during VM shutdown 
        oldStatus = THREAD_UNDEFINED;  // shut up gcc
    } 

    dvmLockMutex(pMutex); 
  /*  
    if (self != NULL)
        self->status = oldStatus;
    */
}

void abcUnlockMutex(pthread_mutex_t* pMutex){
    dvmUnlockMutex(pMutex);
}

void addStartToTrace(int opId){
    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    op->opType = ABC_START;
    op->tid = dvmThreadSelf()->abcThreadId;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));
    
    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " START" << "\n";
    outfile.close(); 
}

//a test trace. this is not called anywhere by default except when 
//a small testcase is required
void executeTestcase(){
    Object* obj = new Object;
    Object* obj1 = new Object;
    //testing graph generation

    addForkToTrace(abcOpCount++, 1, 3);
    addAttachQToTrace(abcOpCount++, 1, 1);

    addNativeEntryToTrace(abcOpCount++, 2);
    addPostToTrace(abcOpCount++, 2, 1, 1, 0);
    addNativeExitToTrace(abcOpCount++, 2);

    addNativeEntryToTrace(abcOpCount++, 2);
    addPostToTrace(abcOpCount++, 2, 2, 1, 0);
    addNativeExitToTrace(abcOpCount++, 2);

    addLoopToTrace(abcOpCount++, 1, 1);

    addThreadInitToTrace(abcOpCount++, 3);
    addLockToTrace(abcOpCount++, 3, obj);
    addReadWriteToTrace(1, ABC_WRITE, "abcClazz", NULL, 4, obj1, "", 3);
    addUnlockToTrace(abcOpCount++, 3, obj);
    addThreadExitToTrace(abcOpCount++, 3);

    addCallToTrace(abcOpCount++, 1, 1);
    addLockToTrace(abcOpCount++, 1, obj);
    addReadWriteToTrace(2, ABC_WRITE, "abcClazz", NULL, 4, obj1, "", 1);
    addUnlockToTrace(abcOpCount++, 1, obj);
    addRetToTrace(abcOpCount++, 1, 1);

    addCallToTrace(abcOpCount++, 1, 2);
    addLockToTrace(abcOpCount++, 1, obj);
    addReadWriteToTrace(3, ABC_READ, "abcClazz", NULL, 4, obj1, "", 1);
    addUnlockToTrace(abcOpCount++, 1, obj);
    addRetToTrace(abcOpCount++, 1, 2);

    addThreadExitToTrace(abcOpCount++, 1);
}

void startAbcModelChecker(){
    gAbc = new AbcGlobals;
    pthread_cond_init(&gAbc->abcMainCond, NULL);
    dvmInitMutex(&gAbc->abcMainMutex);
    abcStartOpId = abcOpCount;
    addStartToTrace(abcOpCount++);
    addThreadInitToTrace(abcOpCount++, dvmThreadSelf()->abcThreadId); 
    char * component = new char[2];
    strcpy(component, "");
    component[1] = '\0';
    addEnableLifecycleToTrace(abcOpCount++, dvmThreadSelf()->abcThreadId, component, 0, ABC_BIND);

    //initialize UI widget class set
    UiWidgetSet.insert("Landroid/view/View;");
    UiWidgetSet.insert("Landroid/widget/TextView;");
    UiWidgetSet.insert("Landroid/widget/ImageView;");
    UiWidgetSet.insert("Landroid/widget/Button;");
    UiWidgetSet.insert("Landroid/widget/AutoCompleteTextView;");
    UiWidgetSet.insert("Landroid/widget/ToggleButton;");
    UiWidgetSet.insert("Landroid/widget/RadioButton;");
    UiWidgetSet.insert("Landroid/widget/EditText;");
    UiWidgetSet.insert("Landroid/webkit/WebView;");
    UiWidgetSet.insert("Landroid/webkit/WebTextView;");
    UiWidgetSet.insert("Landroid/widget/Chronometer;");
    UiWidgetSet.insert("Landroid/widget/Switch;");
    UiWidgetSet.insert("Landroid/widget/CheckBox;");
    UiWidgetSet.insert("Landroid/widget/CheckedTextView;");
    UiWidgetSet.insert("Landroid/widget/DigitalClock;");
    UiWidgetSet.insert("Landroid/widget/AnalogClock;");
    UiWidgetSet.insert("Landroid/widget/ExpandableListView;");
    UiWidgetSet.insert("Landroid/widget/ListView;");
    UiWidgetSet.insert("Landroid/widget/Spinner;");
    UiWidgetSet.insert("Landroid/widget/Gallery;");
    UiWidgetSet.insert("Landroid/widget/GridLayout;");
    UiWidgetSet.insert("Landroid/widget/GridView;");
    UiWidgetSet.insert("Landroid/widget/TableRow;");
    UiWidgetSet.insert("Landroid/widget/TableLayout;");
    UiWidgetSet.insert("Landroid/widget/FrameLayout;");
    UiWidgetSet.insert("Landroid/widget/LinearLayout;");
    UiWidgetSet.insert("Landroid/widget/RelativeLayout;");
    UiWidgetSet.insert("Landroid/widget/ImageButton;");
    UiWidgetSet.insert("Landroid/widget/ViewAnimator;");
    UiWidgetSet.insert("Landroid/widget/ViewFlipper;");
    UiWidgetSet.insert("Landroid/widget/MediaController;");
    UiWidgetSet.insert("Landroid/widget/NumberPicker;");
    UiWidgetSet.insert("Landroid/widget/CalendarView;");
    UiWidgetSet.insert("Landroid/widget/DatePicker;");
    UiWidgetSet.insert("Landroid/widget/TimePicker;");
    UiWidgetSet.insert("Landroid/widget/DateTimeView;");
    UiWidgetSet.insert("Landroid/widget/SeekBar;");
    UiWidgetSet.insert("Landroid/widget/RadioGroup;");
    UiWidgetSet.insert("Landroid/widget/RatingBar;");
    UiWidgetSet.insert("Landroid/widget/ScrollView;");
    UiWidgetSet.insert("Landroid/widget/SearchView;");
    UiWidgetSet.insert("Landroid/widget/TabHost;");
    UiWidgetSet.insert("Landroid/widget/TabWidget;");
    UiWidgetSet.insert("Landroid/widget/Toast;");
    
}

std::string getLifecycleForCode(int code, std::string lifecycle){
    switch(code){
        case 1: lifecycle = "PAUSE-ACT" ; break;
        case 2: lifecycle = "RESUME-ACT" ; break;
        case 3: lifecycle = "LAUNCH-ACT" ; break;
        case 4: lifecycle = "BIND-APP" ; break;
        case 5: lifecycle = "RELAUNCH-ACT" ; break;
        case 6: lifecycle = "DESTROY-ACT" ; break;
        case 7: lifecycle = "CHANGE-CONFIG" ; break;
        case 8: lifecycle = "STOP-ACT" ; break;
        case 9: lifecycle = "RESULT-ACT" ; break;
        case 10: lifecycle = "CHANGE-ACT-CONFIG" ; break;
        case 11: lifecycle = "CREATE-SERVICE" ; break;
        case 12: lifecycle = "STOP-SERVICE" ; break;
        case 13: lifecycle = "BIND-SERVICE" ; break;
        case 14: lifecycle = "UNBIND-SERVICE" ; break;
        case 15: lifecycle = "SERVICE-ARGS" ; break;
        case 16: lifecycle = "BINDAPP-DONE"; break;
        case 17: lifecycle = "SERVICE_CONNECT"; break;
        case 18: lifecycle = "RUN_TIMER_TASK"; break;
        case 19: lifecycle = "REQUEST_START_SERVICE"; break;
        case 20: lifecycle = "REQUEST_BIND_SERVICE"; break;
        case 21: lifecycle = "REQUEST_STOP_SERVICE"; break;
        case 22: lifecycle = "REQUEST_UNBIND_SERVICE"; break;
        case 23: lifecycle = "START-ACT"; break;
        case 24: lifecycle = "NEW_INTENT"; break;
        case 25: lifecycle = "START_NEW_INTENT"; break;
        case 26: lifecycle = "ABC_REGISTER_RECEIVER";break;
        case 27: lifecycle = "ABC_SEND_BROADCAST";break;
        case 28: lifecycle = "ABC_SEND_STICKY_BROADCAST";break;
        case 29: lifecycle = "ABC_ONRECEIVE";break;
        case 30: lifecycle = "ABC_UNREGISTER_RECEIVER";break;
        case 31: lifecycle = "ABC_REMOVE_STICKY_BROADCAST";break;
        case 32: lifecycle = "ABC_TRIGGER_ONRECIEVE_LATER";break;
        default: lifecycle = "UNKNOWN";
    }
    
    return lifecycle;
}

bool addIntermediateReadWritesToTrace(int opId, int tid){
    bool accessSetAdded = false;
    std::map<int, int>::iterator accessIt = abcThreadAccessSetMap.find(tid);
    if(accessIt != abcThreadAccessSetMap.end()){
        addAccessToTrace(opId, tid, accessIt->second);
        abcThreadAccessSetMap.erase(accessIt);
        accessSetAdded = true;
    }

    return accessSetAdded;
}

void abcAddWaitOpToTrace(int opId, int tid, int waitingThreadId){
    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Enter - Add WAIT to trace", opId);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));

    op->opType = ABC_WAIT;
    op->arg1 = waitingThreadId;
    op->tid = tid;
    op->arg2 = NULL;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));
    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " WAIT tid:" << waitingThreadId <<"\n";
    outfile.close();

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
}

void abcAddNotifyToTrace(int opId, int tid, int notifiedTid){
    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Enter - Add NOTIFY to trace", opId);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));

    op->opType = ABC_NOTIFY;
    op->arg1 = notifiedTid;
    op->tid = tid;
    op->arg2 = NULL;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));
    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " NOTIFY tid:" << tid << " notifiedTid:" << notifiedTid << "\n";
    outfile.close();

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
}

void addAccessToTrace(int opId, int tid, u4 accessId){
    LOGE("%d ABC:Entered - Add ACCESS to trace", opId);
    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = NULL;
    arg2->id = accessId;

    op->opType = ABC_ACCESS;
    op->arg1 = tid;
    op->arg2 = arg2;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::map<int, std::pair<int, std::list<int> > >::iterator 
        absIter = abcRWAbstractionMap.find(accessId);
    if(absIter != abcRWAbstractionMap.end()){
        absIter->second.first = opId;
    }else{
        LOGE("ABC-MISSING: accessId %d entry missing in abcRWAbstractionMap" 
             " when adding ACCESS to trace", accessId);
    }
    
    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " ACCESS tid:" << tid << "\t accessId:"
       << accessId << "\n";
    outfile.close(); 

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
}

void addTriggerServiceLifecycleToTrace(int opId, int tid, char* component, u4 componentId, int state){
    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Enter - Add TRIGGER-SERVICE to trace tid:%d  state: %d", opId, tid, state);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = NULL;
    arg2->id = componentId;

    op->opType = ABC_TRIGGER_SERVICE;
    op->arg1 = state;
    op->arg2 = arg2;
    op->arg3 = -1;
    op->arg4 = -1;
    op->arg5 = new char[strlen(component) + 1];
    strcpy(op->arg5, component);
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));
    std::string lifecycle("");
    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " TRIGGER-SERVICE tid:" << tid << " component:" << component
        << " id:" << componentId << " state:" << getLifecycleForCode(state, lifecycle) <<"\n";
    outfile.close();

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
}

void addTriggerBroadcastLifecycleToTrace(int opId, int tid, char* action, u4 componentId, 
        int intentId, int state, int delayTriggerOpid){
    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Enter - Add TRIGGER-BROADCAST to trace tid:%d  state: %d", opId, tid, state);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = NULL;
    arg2->id = componentId;

    op->opType = ABC_TRIGGER_RECEIVER;
    op->arg1 = state;
    op->arg2 = arg2;
    op->arg3 = intentId;
    op->arg4 = delayTriggerOpid;
    op->arg5 = new char[strlen(action) + 1];
    strcpy(op->arg5, action);
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));
    std::string lifecycle("");
    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " TRIGGER-BROADCAST tid:" << tid << " action:" << action
        << " component:" << componentId << " intent:"<< intentId << " onRecLater:" << delayTriggerOpid
        << " state:" << getLifecycleForCode(state, lifecycle) <<"\n";
    outfile.close();

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
}

void addEnableLifecycleToTrace(int opId, int tid, char* component, u4 componentId, int state){
    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Enter - Add ENABLE-LIFECYCLE to trace", opId);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = NULL;
    arg2->id = componentId;

    op->opType = ABC_ENABLE_LIFECYCLE;
    op->arg1 = state;
    op->arg2 = arg2;
    op->arg3 = -1;
    op->arg4 = -1;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));
    std::string lifecycle("");
    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " ENABLE-LIFECYCLE tid:" << tid << " component:" << component
        << " id:" << componentId << " state:" << getLifecycleForCode(state, lifecycle) <<"\n";
    outfile.close(); 

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
  #ifdef SALMAN_CODE
  enable(tid,NA);
  #endif
}
void addTriggerLifecycleToTrace(int opId, int tid, char* component, u4 componentId, int state){
    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Enter - Add TRIGGER-LIFECYCLE to trace tid:%d  state: %d", opId, tid, state);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = NULL;
    arg2->id = componentId;

    op->opType = ABC_TRIGGER_LIFECYCLE;
    op->arg1 = state;
    op->arg2 = arg2;
    op->arg3 = -1;
    op->arg4 = -1;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));
    std::string lifecycle("");
    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " TRIGGER-LIFECYCLE tid:" << tid << " component:" << component
        << " id:" << componentId << " state:" << getLifecycleForCode(state, lifecycle) <<"\n";
    outfile.close(); 

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
}

void addInstanceIntentMapToTrace(int opId, int tid, u4 instance, int intentId){
    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Enter - Add INSTANCE-INTENT to trace tid:%d ", opId, tid);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = NULL;
    arg2->id = instance;

    op->opType = ABC_INSTANCE_INTENT;
    op->arg1 = intentId;
    op->arg2 = arg2;
    op->arg3 = -1;
    op->arg4 = -1;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));
    std::string lifecycle("");
    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " INSTANCE-INTENT tid:" << tid << " instance:" << instance
        << " intentId:" << intentId <<"\n";
    outfile.close();

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
}

void addEnableEventToTrace(int opId, int tid, u4 view, int event){
    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Enter - Add ENABLE-EVENT to trace", opId);
   
    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = NULL;
    arg2->id = view;

    op->opType = ABC_ENABLE_EVENT;
    op->arg1 = event;
    op->arg2 = arg2;
    op->arg3 = -1;
    op->arg4 = -1;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " ENABLE-EVENT tid:" << tid << " view:" << view 
        << " event:" << event <<"\n";
    outfile.close(); 

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
   #ifdef SALMAN_CODE
   enable(tid,NA);
   #endif
}

void addTriggerEventToTrace(int opId, int tid, u4 view, int event){
    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Enter - Add TRIGGER-EVENT to trace", opId);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = NULL;
    arg2->id = view;

    op->opType = ABC_TRIGGER_EVENT;
    op->arg1 = event;
    op->arg2 = arg2;
    op->arg3 = -1;
    op->arg4 = -1;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " TRIGGER-EVENT tid:" << tid << " view:" << view
        << " event:" << event <<"\n";
    outfile.close(); 

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
}

void addEnableWindowFocusChangeEventToTrace(int opId, int tid, u4 windowHash){
    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Enter - Add ENABLE-WINDOW-FOCUS to trace", opId);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = NULL;
    arg2->id = windowHash;

    op->opType = ENABLE_WINDOW_FOCUS;
    op->arg1 = -1;
    op->arg2 = arg2;
    op->arg3 = -1;
    op->arg4 = -1;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " ENABLE-WINDOW-FOCUS tid:" << tid 
        << " windowHash:" << windowHash <<"\n";
    outfile.close();

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
}

void addTriggerWindowFocusChangeEventToTrace(int opId, int tid, u4 windowHash){
    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Enter - Add TRIGGER-WINDOW-FOCUS to trace", opId);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = NULL;
    arg2->id = windowHash;

    op->opType = TRIGGER_WINDOW_FOCUS;
    op->arg1 = -1;
    op->arg2 = arg2;
    op->arg3 = -1;
    op->arg4 = -1;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " TRIGGER-WINDOW-FOCUS tid:" << tid 
        << " windowHash:" << windowHash <<"\n";
    outfile.close();

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
}

int addPostToTrace(int opId, int srcTid, u4 msg, int destTid, s8 delay){

    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, srcTid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Entered - Add POST to trace", opId);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = NULL;
    arg2->id = msg;
 
    op->opType = ABC_POST;
    op->arg1 = srcTid;
    op->arg2 = arg2;
    op->arg3 = destTid;
    op->arg4 = delay;
    op->tid = srcTid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " POST src:" << srcTid << " msg:" << msg 
        << " dest:" << destTid << " delay:" << delay <<"\n";
    outfile.close(); 

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
   #ifdef SALMAN_CODE
    int first;
    if(delay==-1){
                   first=1;
                   delay=0;
                 }
    else {
          first=0;
         }
    post(srcTid,NA,msg,abs(destTid),delay,first);
    #endif
    return opId;
}

void addCallToTrace(int opId, int tid, u4 msg){
    LOGE("%d ABC:Entered - Add CALL to trace", opId);
    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = NULL;
    arg2->id = msg;

    op->opType = ABC_CALL;
    op->arg1 = tid;
    op->arg2 = arg2;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " CALL tid:" << tid << "\t msg:" << msg  << "\n";
    outfile.close(); 

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
   #ifdef SALMAN_CODE
     task_begin(tid,msg);
   #endif

}

void addRetToTrace(int opId, int tid, u4 msg){

    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Entered - Add RET to trace", opId);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = NULL;
    arg2->id = msg;
    
    op->opType = ABC_RET;
    op->arg1 = tid;
    op->arg2 = arg2;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " RET tid:" << tid << "\t msg:" << msg  << "\n";
    outfile.close(); 

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
    #ifdef SALMAN_CODE
     task_end(tid,msg);
    #endif
} 

void addRemoveToTrace(int opId, int tid, u4 msg){
    LOGE("ABC:Entered - Add REMOVE to trace");
    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = NULL;
    arg2->id = msg;

    op->opType = ABC_REM;
    op->arg1 = tid;
    op->arg2 = arg2;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
}

void addAttachQToTrace(int opId, int tid, u4 msgQ){

    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Entered - Add ATTACHQ to trace", opId);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = NULL;
    arg2->id = msgQ;

    op->opType = ABC_ATTACH_Q;
    op->arg1 = tid;
    op->arg2 = arg2;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " ATTACH-Q tid:" << tid << "\t queue:" << msgQ <<"\n";
    outfile.close(); 

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
   #ifdef SALMAN_CODE
    attachq(tid);
   #endif
}

void addLoopToTrace(int opId, int tid, u4 msgQ){

    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Entered - Add LOOP to trace", opId);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = NULL;
    arg2->id = msgQ;

    op->opType = ABC_LOOP;
    op->arg1 = tid;
    op->arg2 = arg2;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " LOOP tid:" << tid << "\t queue:" << msgQ <<"\n";
    outfile.close();

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
   #ifdef SALMAN_CODE
    looponq(tid);
   #endif   
}

void addQueueIdleToTrace(int opId, u4 idleHandlerHash, int queueHash, int tid){
    LOGE("%d ABC:Entered - Add QUEUE_IDLE to trace", opId);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->id = idleHandlerHash;
    arg2->obj = NULL;

    op->opType = ABC_QUEUE_IDLE;
    op->arg1 = queueHash;
    op->arg2 = arg2;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " QUEUE_IDLE tid:" << tid << " idler:" << idleHandlerHash << " queue:" << queueHash <<"\n";
    outfile.close();

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
}

void addIdleHandlerToTrace(int opId, u4 idleHandlerHash, int queueHash, int tid){
    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Entered - Add ADD_IDLE_HANDLER to trace", opId);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->id = idleHandlerHash;
    arg2->obj = NULL;

    op->opType = ABC_ADD_IDLE_HANDLER;
    op->arg1 = queueHash;
    op->arg2 = arg2;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " ADD_IDLE_HANDLER idler:" << idleHandlerHash << " queue:" << queueHash <<"\n";
    outfile.close();

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
}

void addRemoveIdleHandlerToTrace(int opId, u4 idleHandlerHash, int queueHash, int tid){
    LOGE("%d ABC:Entered - Add REMOVE_IDLE_HANDLER to trace", opId);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->id = idleHandlerHash;
    arg2->obj = NULL;

    op->opType = ABC_REMOVE_IDLE_HANDLER;
    op->arg1 = queueHash;
    op->arg2 = arg2;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " REMOVE_IDLE_HANDLER idler:" << idleHandlerHash << " queue:" << queueHash <<"\n";
    outfile.close();

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
}

void addLockToTrace(int opId, int tid, Object* lockObj){

    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Entered - Add LOCK to trace obj:%p tid:%d", opId, lockObj, tid);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = lockObj;
    arg2->id = 0;

    op->opType = ABC_LOCK;
    op->arg1 = tid;
    op->arg2 = arg2;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " LOCK" << " tid:" << tid << "\t lock-obj:" << lockObj << "\n";
    outfile.close(); 
    LOGD("lock value %d",lockObj->lock);
    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
 #ifdef SALMAN_CODE
    droid_lock(tid,(int)lockObj);
 #endif
}

void addUnlockToTrace(int opId, int tid, Object* lockObj){

    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Entered - Add UNLOCK to trace obj:%p tid:%d", opId, lockObj, tid);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = lockObj;
    arg2->id = 0;

    op->opType = ABC_UNLOCK;
    op->arg1 = tid;
    op->arg2 = arg2;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));
    /*
    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " UNLOCK" << " tid:" << tid << "\t lock-obj:" << lockObj << "\n";
    outfile.close(); 
    */
    LOGD("Unlock value %d",lockObj->lock);
    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
   #ifdef SALMAN_CODE
    unlock(tid,(int)lockObj);
   #endif
}

void addForkToTrace(int opId, int parentTid, int childTid){

    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, parentTid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Entered - Add FORK to trace", opId);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    AbcArg* arg2 = (AbcArg*)malloc(sizeof(AbcArg));
    arg2->obj = NULL;
    arg2->id = childTid;

    op->opType = ABC_FORK;
    op->arg1 = parentTid;
    op->arg2 = arg2;
    op->tid = parentTid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " FORK par-tid:" << parentTid << "\t child-tid:"
        << childTid << "\n";
    outfile.close(); 

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
    #ifdef SALMAN_CODE
      thread_create(parentTid,childTid);
    #endif
}

void addThreadInitToTrace(int opId, int tid){
    LOGE("%d ABC:Entered - Add THREADINIT to trace", opId);
    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));

    op->opType = ABC_THREADINIT;
    op->arg1 = tid;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " THREADINIT tid:" << tid << "\n";
    outfile.close(); 

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
    #ifdef SALMAN_CODE
      thread_init(tid,0);
    #endif
}

void addThreadExitToTrace(int opId, int tid){

    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }
    LOGE("%d ABC:Entered - Add THREADEXIT to trace", opId);

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    op->opType = ABC_THREADEXIT;
    op->arg1 = tid;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " THREADEXIT tid:" << tid << "\n";
    outfile.close(); 

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
    #ifdef SALMAN_CODE
       thread_exit(tid);
    #endif
}

void addNativeEntryToTrace(int opId, int tid){
    LOGE("%d ABC:Entered - Add NATIVE_ENTRY to trace", opId);
    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));

    op->opType = ABC_NATIVE_ENTRY;
    op->arg1 = tid;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));

    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " NATIVE-ENTRY tid:" << tid << "thread-name:" << dvmGetThreadName(dvmThreadSelf()).c_str() << "\n";
    outfile.close(); 

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
}

void addNativeExitToTrace(int opId, int tid){
    LOGE("%d ABC:Entered - Add NATIVE_EXIT to trace", opId);

    bool accessSetAdded = addIntermediateReadWritesToTrace(opId, tid);
    if(accessSetAdded){
        opId = abcOpCount++;
    }

    AbcOp* op = (AbcOp*)malloc(sizeof(AbcOp));
    op->opType = ABC_NATIVE_EXIT;
    op->arg1 = tid;
    op->tid = tid;
    op->tbd = false;
    op->asyncId = -1;

    abcTrace.insert(std::make_pair(opId, op));
    
    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << opId << " NATIVE-EXIT tid:" << tid << "thread-name:" << dvmGetThreadName(dvmThreadSelf()).c_str() << "\n";
    outfile.close(); 

    if(abcTraceLengthLimit != -1 && opId >= abcTraceLengthLimit){
        gDvm.isRunABC = false;
        LOGE("Trace truncated as hit trace limit set by user");
    }
}

void addReadWriteToTrace(int rwId, int accessType, const char* clazz, std::string field, u4 fieldIdx, Object* obj, std::string dbPath, int tid){ 
    /* store fieldnames belonging to this class if not already stored. Will be useful for debugging*/
    if(obj != NULL){
        bool foundField = false;

        const char* tmpClassName = clazz;
        ClassObject* tmpClassObj = obj->clazz; 

        while(!foundField){
          AbcClassField* abcClazz = abcGetClassIfInClassFieldsMap(tmpClassName); 

          if(abcClazz != NULL){
            u4 tmpMinOffset = abcClazz->firstFieldOffset;
            if(tmpMinOffset <= fieldIdx){
                foundField = true; //fieldIdx present in abcClazz whose fields are already stored
            }else{
                if(tmpClassObj->super != NULL){
                    tmpClassName = tmpClassObj->super->descriptor;
                    tmpClassObj = tmpClassObj->super;
                }else{
                    LOGE("ABC: We hit an access to field %d belonging to super class (may be java/lang/Object) of class %s.",fieldIdx,clazz);
                    foundField = true;
                }
            }
          }else{
            abcClazz = abcAddAndGetClassToClassFieldsMap(tmpClassName, -1);
            if(abcClazz != NULL){
              //LOGE("ABC: Class to store %s", tmpClassName);
              int minOffset = INT_MAX;
              for (int i = 0; i < tmpClassObj->ifieldCount; i++) {
                  InstField* pField = &tmpClassObj->ifields[i]; 
                  if(pField->byteOffset < minOffset){
                      minOffset = pField->byteOffset;
                  } 
                  abcStoreFieldNameForClass(abcClazz, pField->name, pField->byteOffset);
                  //LOGE("ABC: storing fieldname: %s for byteOffset:%d", pField->name, pField->byteOffset);
              }
              abcClazz->firstFieldOffset = minOffset;
              u4 tmpVar = abcClazz->firstFieldOffset;
              if(tmpVar <= fieldIdx)
                  foundField = true;

              /*if fieldIdx < minOffSet then fieldIdx belongs to a super class*/
              if(!foundField){
                  if(tmpClassObj->super != NULL){
                      tmpClassName = tmpClassObj->super->descriptor;
                      tmpClassObj = tmpClassObj->super;
                  }else{
                      LOGE("ABC: We hit an access to field %d belonging to super class (may be java/lang/Object) of class %s.",fieldIdx,clazz);
                      foundField = true;
                  }
              }
            
            }else{
              /* set foundField to true to exit loop. May be we hit memory issues and thus
               * unable to allocate AbcClassFieldObject
               */
              foundField = true;
              LOGE("ABC-ERROR: could not instantiate AbcClassField object when we hit a new Class");
            }
          }
        }
    }

    AbcRWAccess* access = (AbcRWAccess*)malloc(sizeof(AbcRWAccess));
    access->accessType = accessType;
    access->obj = obj;
    access->dbPath = new char[dbPath.size() + 1];
    std::copy(dbPath.begin(), dbPath.end(), access->dbPath);
    access->dbPath[dbPath.size()] = '\0';
    access->clazz = clazz;
    if(obj == NULL){
        access->field = new char[field.size() + 1];
        std::copy(field.begin(), field.end(), access->field);
        access->field[field.size()] = '\0';
    }else{
        std::string fieldName;
        fieldName.assign(abcGetFieldNameOfClassFieldId(obj, fieldIdx));
        access->field = new char[fieldName.size() + 1];
        std::copy(fieldName.begin(), fieldName.end(), access->field);
        access->field[fieldName.size()] = '\0';
    }
    access->fieldIdx = fieldIdx;
    access->tid = tid;
 
    std::map<int, int>::iterator accessIt = abcThreadAccessSetMap.find(tid);
    std::map<int, std::pair<int, std::list<int> > >::iterator rwIt;
    if(accessIt == abcThreadAccessSetMap.end()){
        //if this is the first read/write seen after a non access operation
        abcThreadAccessSetMap.insert(std::make_pair(tid, abcAccessSetCount++)); 
        std::list<int> tmpList; 
        abcRWAbstractionMap.insert(make_pair(abcAccessSetCount-1, std::make_pair(-1, tmpList)));
        rwIt = abcRWAbstractionMap.find(abcAccessSetCount-1);
    }else{
        rwIt = abcRWAbstractionMap.find(accessIt->second);
    }
    rwIt->second.second.push_back(rwId);

    access->accessId = rwIt->first;
    abcRWAccesses.insert(std::make_pair(rwId, access));


    //insert this operation to appropriate access tracker map
    if(obj != NULL){
        std::pair<Object*, u4> tmpPair = std::make_pair(obj, fieldIdx);
        std::map<std::pair<Object*, u4>, std::pair<std::set<int>, std::set<int> > >::iterator it 
            = abcObjectAccessMap.find(tmpPair);
        if(it != abcObjectAccessMap.end()){
            if(accessType == ABC_READ)
                it->second.first.insert(rwId);
            else
                it->second.second.insert(rwId);
        }else{
            std::set<int> readSet;
            std::set<int> writeSet;
            if(accessType == ABC_READ)
                readSet.insert(rwId);
            else
                writeSet.insert(rwId);
            abcObjectAccessMap.insert(std::make_pair(tmpPair, std::make_pair(readSet, writeSet)));
        }
    }else if(dbPath != ""){
        std::map<std::string, std::pair<std::set<int>, std::set<int> > >::iterator it  
            = abcDatabaseAccessMap.find(dbPath);
        if(it != abcDatabaseAccessMap.end()){
            if(accessType == ABC_READ)
                it->second.first.insert(rwId);
            else
                it->second.second.insert(rwId);
        }else{  
            std::set<int> readSet;
            std::set<int> writeSet;
            if(accessType == ABC_READ)
                readSet.insert(rwId);
            else
                writeSet.insert(rwId);
            abcDatabaseAccessMap.insert(std::make_pair(std::string(access->dbPath),std::make_pair(readSet, writeSet)));
        }  
    }else{
        std::pair<const char*, u4> tmpPair = std::make_pair(clazz, fieldIdx);
        std::map<std::pair<const char*, u4>, std::pair<std::set<int>, std::set<int> > >::iterator it
            = abcStaticAccessMap.find(tmpPair);
        if(it != abcStaticAccessMap.end()){
            if(accessType == ABC_READ)
                it->second.first.insert(rwId);
            else
                it->second.second.insert(rwId);
        }else{
            std::set<int> readSet;
            std::set<int> writeSet;
            if(accessType == ABC_READ)
                readSet.insert(rwId);
            else
                writeSet.insert(rwId);
            abcStaticAccessMap.insert(std::make_pair(tmpPair,std::make_pair(readSet, writeSet)));
        }
    }
}

WorklistElem* getNextEdgeFromWorklist(){
    WorklistElem* edge = worklist;
    if(worklist != NULL)
        worklist = worklist->prev;
    
    return edge;
}

//adding only when processing cild async's call edge. if parent present on
//same thread then parent's ret is guaranteed to have been seen.
void addAsyncPostEdge(int childOp, AbcAsync* child){
    if(child->parentAsyncId != -1){
        std::map<int, AbcAsync*>::iterator asyncIter = abcAsyncMap.find(child->parentAsyncId);
        if(asyncIter != abcAsyncMap.end()){
            AbcAsync* parent = asyncIter->second;
            if(parent->tid == child->tid){
                addEdgeToHBGraph(parent->retId, childOp);
            }
        }else{
            LOGE("ABC-MISSING: async block missing in abcAsyncMap for asyncId %d",
                  child->parentAsyncId);
        }
    }
    addEdgeToHBGraph(child->postId, childOp);
}

int getSourceOfFollowsEdgeToNode(int opId){
    int sourceOp = -1;

    return sourceOp;
}

void addFollowsEdgeFromPrevToNextOp(int opId){

}

bool checkAndAbortIfAssumtionsFailed(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
    bool shouldAbort = false;
    if(threadBK->loopId != -1 && op->asyncId == -1){
        switch(op->opType){
            case ABC_ACCESS:
            case ABC_LOCK:
            case ABC_UNLOCK:
            case ABC_FORK:
            case ABC_JOIN: 
                shouldAbort = true; 
                LOGE("Trace has an unusual sequence op operations outside async blocks, which is not addressed by "
                "implementation. Cannot continue further");
                 break;
        }
    }
    return shouldAbort;
}

bool processPostOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
//    LOGE("ABC: processing POST opid:%d msg:%d", opId, op->arg2->id);
    bool shouldAbort = false;
    op->asyncId = threadBK->curAsyncId; //-1 if posted from non-queue thread

    AbcAsync* async = (AbcAsync*)malloc(sizeof(AbcAsync));
    async->tid = op->arg3; //thread on which async block executes
    async->postId = opId;
    async->parentAsyncId = op->asyncId;
    async->delay = op->arg4;
    
    //initializing fields needed collect nature of race
    if(op->asyncId != -1){
        std::map<int, AbcAsync*>::iterator parIter = abcAsyncMap.find(op->asyncId);
        if(parIter != abcAsyncMap.end()){
            async->recentTriggerOpId = parIter->second->recentTriggerOpId;
            async->recentCrossPostAsync = parIter->second->recentCrossPostAsync;
            async->recentDelayAsync = parIter->second->recentDelayAsync;
        }else{
            LOGE("ABC-MISSING: missing async %d in asyncmap", op->asyncId);
            async->recentTriggerOpId = -1;
            async->recentCrossPostAsync = -1;
            async->recentDelayAsync = -1;
        }
    }else{
        async->recentTriggerOpId = -1;
        async->recentCrossPostAsync = -1;
        async->recentDelayAsync = -1;
    }

    //check if this is a cross-post
    if(op->tid != async->tid){
        async->recentCrossPostAsync = op->arg2->id; //itself
    }
    //check if this is a delayed post
    if(async->delay > 0){
        async->recentDelayAsync = op->arg2->id; //itself
    }

    //a heuristic used currently to discard traces with front-of-Q msgs
    if(async->delay == -1){
        LOGE("Found a front-of-Q msg during trace processing (which was supposed"
                " to be deleted), which is not addressed by "
                "implementation. Cannot continue further");
        shouldAbort = true;
        return shouldAbort;
    }

    async->callId = -1;
    async->retId = -1;

    //msgId becomes the async block id
    abcAsyncMap.insert(std::make_pair(op->arg2->id, async));
   
    //a post generated outside async block after the looper starts looping
    if(threadBK->loopId != -1 && op->asyncId == -1){
        addEdgeToHBGraph(threadBK->loopId, opId);
    }
    
    return shouldAbort;
}

bool processCallOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
    bool shouldAbort = false;
    threadBK->curAsyncId = op->arg2->id; //msg id of call operation
    op->asyncId = threadBK->curAsyncId;
    
    std::map<int, AbcAsync*>::iterator asyncIter = abcAsyncMap.find(op->asyncId);
    if(asyncIter != abcAsyncMap.end()){
        AbcAsync* async = asyncIter->second; //get async block entry stored by POST
        async->callId = opId;
    //    LOGE("ABC:processing CALL opid :%d   msg:%d postId:%d", opId, op->arg2->id, async->postId);

        addAsyncPostEdge(opId, async);
    }else{
        LOGE("ABC-MISSING: entry for asyncId %d missing when hit CALL", op->asyncId);
    }

    //ATTACH-Q rule 
    if(threadBK->loopId == -1){
        LOGE("ABC: CALL seen on a loop before LOOP operation. Aborting.");
        shouldAbort = true;
    }else{
        addEdgeToHBGraph(threadBK->loopId, opId);
    }
    return shouldAbort;
}

void processRetOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
//    LOGE("ABC: processing RET opid:%d", opId);
    op->asyncId = threadBK->curAsyncId;
    std::map<int, AbcAsync*>::iterator asyncIter = abcAsyncMap.find(op->asyncId);
    if(asyncIter != abcAsyncMap.end()){
        AbcAsync* async = asyncIter->second; 
        async->retId = opId;
    }else{
        LOGE("ABC-MISSING: entry for asyncId %d missing when hit RET", op->asyncId);
    }
    threadBK->curAsyncId = -1;
}

void processRemoveOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
    std::map<int, AbcAsync*>::iterator it = abcAsyncMap.find(op->arg2->id);
    if(it != abcAsyncMap.end()){
        std::map<int, AbcOp*>::iterator traceIt = abcTrace.find(it->second->postId);
        if(traceIt != abcTrace.end()){
            abcTrace.erase(traceIt);
            if(threadBK->prevOpId == it->second->postId){
                int src = getSourceOfFollowsEdgeToNode(threadBK->prevOpId);
                threadBK->prevOpId = src;
            }
            addFollowsEdgeFromPrevToNextOp(opId);
        }
        abcAsyncMap.erase(it); 
    }
}

void processAttachqOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
//    LOGE("ABC: processing ATTACH-Q opid:%d", opId);
    threadBK->attachqId = opId;
    abcMQCount++;
}

void processLoopOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
//    LOGE("ABC: processing LOOP opid:%d", opId);
    threadBK->loopId = opId;
    addEdgeToHBGraph(threadBK->attachqId, threadBK->loopId);
}

void processAddIdleHandlerOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
    op->asyncId = threadBK->curAsyncId; 
    AbcAsync* prevAsync = NULL;
    abcIdleHandlerMap.insert(std::make_pair(std::make_pair(op->arg2->id, op->arg1), 
            std::make_pair(opId, prevAsync)));
}

void processQueueIdleOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
    op->asyncId = threadBK->curAsyncId;
    AbcAsync* opAsync = getAsyncBlockFromId(op->asyncId);
    opAsync->recentTriggerOpId = opId;

    std::pair<u4, int> idlerQueuePair = std::make_pair(op->arg2->id, op->arg1);
    std::map<std::pair<u4, int>, std::pair<int, AbcAsync*> >::iterator it = 
            abcIdleHandlerMap.find(idlerQueuePair);
    if(it != abcIdleHandlerMap.end()){
        //add edge from addIdler to queueIdle()'s dummy post
        //dummy post bcos we added this post to be coherent with our semantics
        addEdgeToHBGraph(it->second.first, opAsync->postId);    
        //if addIdler is performed on the same thread and from async block
        //add edge from ret of async block to dummy post of queueIdle
        AbcOp* addIdlerOp = abcTrace.find(it->second.first)->second;
        if(addIdlerOp->asyncId != -1 && addIdlerOp->tid == op->tid){
            AbcAsync* addIdlerAsync = getAsyncBlockFromId(addIdlerOp->asyncId);
            addEdgeToHBGraph(addIdlerAsync->retId, opAsync->postId);
        }

        //if the same idler was called earlier add HB edge between the previous and current
        //this is just a heuristic to avoid reporting uninteresting race
        if(it->second.second != NULL){
            addEdgeToHBGraph(it->second.second->retId, opAsync->postId);
        }
        it->second.second = opAsync;
    }
    
    //queueIdle() can be called only after loop()
    addEdgeToHBGraph(threadBK->loopId, opId);
    addEdgeToHBGraph(threadBK->loopId, opAsync->postId);
}

void processRemoveIdleHandlerOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
    std::pair<u4, int> idlerQueuePair = std::make_pair(op->arg2->id, op->arg1);
    abcIdleHandlerMap.erase(idlerQueuePair);
}

bool processForkOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
//    LOGE("ABC: processing FORK opid:%d", opId);
    bool shouldAbort = false;
    op->asyncId = threadBK->curAsyncId;
    AbcThreadBookKeep* newThread = (AbcThreadBookKeep*)malloc(sizeof(AbcThreadBookKeep));
    newThread->forkId = opId;
    newThread->loopId = -1;
    newThread->curAsyncId = -1;
    newThread->prevOpId = -1;
    newThread->attachqId = -1;
    
    std::pair<std::map<int,AbcThreadBookKeep*>::iterator, bool> res = 
        abcThreadBookKeepMap.insert(std::make_pair(op->arg2->id, newThread));
    if(res.second == false){
        LOGE("ABC: adding a thread to AbcThreadBookKeepMap failed. An entry already "
           "exists for this thread. Aborting processing the trace.");
        shouldAbort = true;
        return shouldAbort;
    }

    return shouldAbort;
}

bool processNotifyOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
    bool shouldAbort = false;
    op->asyncId = threadBK->curAsyncId;
    std::map<int, int>::iterator it = notifyWaitMap.find(op->arg1);
    if(it == notifyWaitMap.end()){
        notifyWaitMap.insert(std::make_pair(op->arg1, opId));
    }else{
        LOGE("ABC-ABORT: another notify %d seen when already one exists without a wait."
             " Aborting due to unknown pattern", op->arg1 );
        shouldAbort = true;
    }

    return shouldAbort;
}

void processWaitOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
//if there exists a corresponding notify connect to this
    op->asyncId = threadBK->curAsyncId;
    std::map<int, int>::iterator it = notifyWaitMap.find(op->arg1);
    if(it != notifyWaitMap.end()){
        //edge from notify to wait 
        /*it is unclear if we need an edge from pre-wait to notify
         *indicating thread adding itself to wait list and notify 
         *triggered only after that
         */
        addEdgeToHBGraph(it->second, opId);   
        notifyWaitMap.erase(it);
    }
}

void processThreadinitOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
//    LOGE("ABC: processing THREADINIT opid:%d", opId);
    threadBK->prevOpId = opId;
    addEdgeToHBGraph(threadBK->forkId, opId);
    addEdgeToHBGraph(abcStartOpId, opId);

    abcForkCount++; //we count the fork only after the thread gets initialized i.e it does dome action
}

void processThreadexitOperation(int opId, AbcOp* op){
//    LOGE("ABC: processing THREADEXIT opid:%d", opId);
    abcThreadBookKeepMap.erase(op->arg1);
}

bool processNativeEntryOperation(int opId, AbcOp* op){
//    LOGE("ABC: processing native entry opid:%d", opId);
    bool shouldAbort = false;
    std::map<int, AbcThreadBookKeep*>::iterator threadIt = abcThreadBookKeepMap.find(op->tid);
    if(threadIt == abcThreadBookKeepMap.end()){
        AbcThreadBookKeep* newThread = (AbcThreadBookKeep*)malloc(sizeof(AbcThreadBookKeep));
        newThread->forkId = -1;
        newThread->loopId = -1;
        newThread->curAsyncId = -1;
        newThread->prevOpId = opId;
        newThread->attachqId = -1;
        abcThreadBookKeepMap.insert(std::make_pair(op->tid, newThread));
    }else if(threadIt->second->prevOpId == -1){
        threadIt->second->prevOpId = opId;
    }else{
        LOGE("ABC: A native thread did not make a native exit. But another entry spotted "
           "for this thread. Aborting processing the trace.");
        shouldAbort = true;
        return shouldAbort;
    }

    addEdgeToHBGraph(abcStartOpId, opId);
    return shouldAbort;
}

void processNativeExitOperation(int opId, AbcOp* op){
    return;
}

void processAccessOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
//    LOGE("ABC: processing ACCESS opid:%d", opId);
    op->asyncId = threadBK->curAsyncId;
}

void processLockOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
//    LOGE("ABC: processing LOCK opid:%d lock %p", opId, op->arg2->obj);
    op->asyncId = threadBK->curAsyncId;
    
    std::map<Object*, AbcLock*>::iterator lockIt = abcLockMap.find(op->arg2->obj); 
    if(lockIt == abcLockMap.end()){
        AbcLock* lock = new AbcLock;    
        lock->lastUnlockTid = -1;
        lock->prevUnlocksSeen = NULL;
        std::pair<std::map<Object*, AbcLock*>::iterator, bool> insert_result = abcLockMap.insert(std::make_pair(op->arg2->obj, lock));

        if(!insert_result.second) {
            LOGE("ABC: lock %p not added to lockmap. result.first=%p", op->arg2->obj, insert_result.first->first);
        } 
        return;
    }
    
    //if this is not the first lock on this object
    AbcLock* lock = lockIt->second;
    if(op->tid != lock->lastUnlockTid){
        //clearing prevLockSet to add current one
        UnlockList* prevUnlockLst = lock->prevUnlocksSeen;
        while(prevUnlockLst != NULL){
            lock->prevUnlocksSeen = prevUnlockLst->prev;
            free(prevUnlockLst);
            prevUnlockLst = lock->prevUnlocksSeen;
        }
        
        std::map<int, UnlockList*>::iterator unlockIter = lock->unlockMap.find(lock->lastUnlockTid);
        if(unlockIter != lock->unlockMap.end()){
            UnlockList* unlockSet = unlockIter->second;
            while(unlockSet != NULL){
            addEdgeToHBGraph(unlockSet->unlockOp, opId);
            if(op->asyncId != -1){
                UnlockList* tmpUnlock = (UnlockList*)malloc(sizeof(UnlockList));
                tmpUnlock->unlockOp = unlockSet->unlockOp;
                tmpUnlock->prev = lock->prevUnlocksSeen;
                lock->prevUnlocksSeen = tmpUnlock;
            }
            
            unlockSet = unlockSet->prev;
        }

        }
    }
    //last unlock tid is same as the current lock tid
    else{
        /*current lock is taken inside an async block and
         *previous is an unlock on the same thread (may or
         *may not be inside async block)
         */
        if(op->asyncId != -1){
            UnlockList* prevUnlockLst = lock->prevUnlocksSeen;
            while(prevUnlockLst != NULL){
                addEdgeToHBGraph(prevUnlockLst->unlockOp, opId);
                prevUnlockLst = prevUnlockLst->prev;
            }
        }
    }
}

bool processUnlockOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
//    LOGE("ABC: processing UNLOCK opid:%d obj:%p", opId, op->arg2->obj);
    bool shouldAbort = false;
    op->asyncId = threadBK->curAsyncId;

    std::map<Object*, AbcLock*>::iterator lockIt = abcLockMap.find(op->arg2->obj);
    if(lockIt == abcLockMap.end()){
       LOGE("ABC: even after a lock, lock object has not been added to abcLockMap."
             " Trace processing aborted.");
       shouldAbort = true;
       return shouldAbort;
    }

    AbcLock* lock = lockIt->second;
    std::map<int, UnlockList*>::iterator it = lock->unlockMap.find(op->tid);
    if(it == lock->unlockMap.end()){
        UnlockList* newUnlockLst = (UnlockList*)malloc(sizeof(UnlockList));
        newUnlockLst->unlockOp = opId;
        newUnlockLst->prev = NULL;
        lock->unlockMap.insert(std::make_pair(op->tid, newUnlockLst));
    }else if(op->asyncId == -1){
        UnlockList* tmpUnlockLst = it->second;
        while(tmpUnlockLst != NULL){
            it->second = tmpUnlockLst->prev;
            free(tmpUnlockLst);
            tmpUnlockLst = it->second;
        }
        UnlockList* newUnlockLst = (UnlockList*)malloc(sizeof(UnlockList));
        newUnlockLst->unlockOp = opId;
        newUnlockLst->prev = NULL;
        it->second = newUnlockLst; 
    }else{
        std::map<int, AbcAsync*>::iterator asyncIter = abcAsyncMap.find(op->asyncId);
        if(asyncIter != abcAsyncMap.end()){
        AbcAsync* async = asyncIter->second;

        UnlockList* tmpUnlockLst = it->second;
        UnlockList* tbd_prev  = NULL;

        while(tmpUnlockLst != NULL){
            int tmpAsyncId = getAsyncIdOfOperation(tmpUnlockLst->unlockOp);
            AbcAsync* tmpAsync = getAsyncBlockFromId(tmpAsyncId);
            if(tmpAsyncId == -1 || tmpAsyncId == op->asyncId ||
                isHbEdge(tmpAsync->retId, async->callId)){
                UnlockList* tbd = tmpUnlockLst;
                if(tbd_prev != NULL) {
                    tbd_prev->prev = tmpUnlockLst->prev;
                }
                tmpUnlockLst = tmpUnlockLst->prev;
                //if head of the list is about to be deleted shift the head to next one
                if(it->second == tbd){
                    it->second = tmpUnlockLst;
                }
                free(tbd);
            }else{
                tbd_prev = tmpUnlockLst;
                tmpUnlockLst = tmpUnlockLst->prev;
            }
        }
        //insert new unlock 
        UnlockList* newUnlockLst = (UnlockList*)malloc(sizeof(UnlockList));
        newUnlockLst->unlockOp = opId;
        newUnlockLst->prev = it->second;
        it->second = newUnlockLst;
        }else{
            LOGE("ABC-MISSING: entry for asyncId %d missing in abcAsyncMap", op->asyncId);
        }
    }

    lock->lastUnlockTid = op->tid;
    return shouldAbort;
}

void processEnableEventOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
    op->asyncId = threadBK->curAsyncId;
    std::pair<u4, int> viewEventPair = std::make_pair(op->arg2->id, op->arg1);
    std::map<std::pair<u4, int>, std::pair<int,int> >::iterator it = abcEnabledEventMap.find(viewEventPair);
    if(it == abcEnabledEventMap.end()){
        abcEnabledEventMap.insert(std::make_pair(viewEventPair, std::make_pair(opId, -1)));
    }else{
        it->second.first = opId;
    }
}

bool processTriggerEventOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
    abcEventTriggerCount++;
    bool shouldAbort = false;
    op->asyncId = threadBK->curAsyncId;

    //needed for race nature stats collection
    AbcAsync* async = getAsyncBlockFromId(op->asyncId);
    async->recentTriggerOpId = opId;
    /*for events post is from native thread and hence this info is not useful
     *as far as possible consider only cross posts due to app code. Also for
     *events connections due to enable-trigger should be of importance and
     *lack of that leading to race should be reflected (categorizing it as
     *co-enabled race or unknown category race)
     */
    async->recentCrossPostAsync = -1; 

    std::pair<u4, int> viewEventPair = std::make_pair(op->arg2->id, op->arg1);
    std::map<std::pair<u4, int>, std::pair<int,int> >::iterator it = abcEnabledEventMap.find(viewEventPair);
    if(it != abcEnabledEventMap.end()){
        it->second.second = opId;
        addEdgeToHBGraph(it->second.first, opId);
        int postId = async->postId;
        //add edge from enable-event to post corresponding to trigger event 
        addEdgeToHBGraph(it->second.first, postId);
        abcAsyncEnableMap.insert(std::make_pair(op->asyncId, it->second.first));
    }else{
        LOGE("ABC: Trigger event seen without a corresponding enable event during processing."
             " Aborting processing.");
        gDvm.isRunABC = false;
        shouldAbort = true;
    }
    return shouldAbort;
}

void processEnableWindowFocusChange(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
    op->asyncId = threadBK->curAsyncId;

    std::map<u4, std::pair<int, int> >::iterator it = abcWindowFocusChangeMap.find(op->arg2->id);
    if(it == abcWindowFocusChangeMap.end()){
        abcWindowFocusChangeMap.insert(std::make_pair(op->arg2->id, std::make_pair(opId, -1)));
    }else{
        it->second.first = opId;
    }
}

void processTriggerWindowFocusChange(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
    op->asyncId = threadBK->curAsyncId;
    //needed for race nature stats collection
    AbcAsync* async = abcAsyncMap.find(op->asyncId)->second;
    async->recentTriggerOpId = opId;
    /*for events post is from native thread and hence this info is not useful
     *as far as possible consider only cross posts due to app code.Also for
     *events connections due to enable-trigger should be of importance and
     *lack of that leading to race should be reflected (categorizing it as
     *co-enabled race or unknown category race)
     */
    async->recentCrossPostAsync = -1;

    //search based on window hash stored on arg2->id
    std::map<u4, std::pair<int, int> >::iterator it = abcWindowFocusChangeMap.find(op->arg2->id);
    if(it != abcWindowFocusChangeMap.end()){
        it->second.second = opId;
        addEdgeToHBGraph(it->second.first, opId);
        int postId = getAsyncBlockFromId(op->asyncId)->postId;
        //add edge from enable-event to post corresponding to trigger event 
        addEdgeToHBGraph(it->second.first, postId);
        abcAsyncEnableMap.insert(std::make_pair(op->asyncId, it->second.first));
    }
}

void processEnableLifecycleOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
//    LOGE("ABC: processing enable lifecycle hit");
    op->asyncId = threadBK->curAsyncId;

    addEnableLifecycleEventToMap(opId, op);
    if(op->arg1 == ABC_RESUME && op->arg2->id == 0){
        blankEnableResumeOp->opId = opId;
        blankEnableResumeOp->opPtr = op; 
    }

    if(op->arg1 == ABC_APPBIND_DONE){
        abcAppBindPost = getAsyncBlockFromId(op->asyncId)->postId;
    }
}

bool processTriggerLifecycleOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
    bool shouldAbort = false;
    op->asyncId = threadBK->curAsyncId;

    //stats needed when reporting races
    abcEventTriggerCount++;
    if(op->asyncId != -1){
        AbcAsync* async = abcAsyncMap.find(op->asyncId)->second;
        async->recentTriggerOpId = opId;
        /*for events post is from native thread and hence this info is not useful
         *as far as possible consider only cross posts due to app code. Also for
         *events connections due to enable-trigger should be of importance and
         *lack of that leading to race should be reflected (categorizing it as
         *co-enabled race or unknown category race)
         */
        async->recentCrossPostAsync = -1;
    }

    /*both edgeAded1 and edgeAdded2 being false mean that you have seen a 
     *TRIGGER with no corresponding ENABLE and neither is at a activity state
     *or, its a activity state but state machine is not consistent with this
     *TRIGGER and hence cant proceed
     */
    bool edgeAdded1, edgeAdded2;
    edgeAdded1 = connectEnableAndTriggerLifecycleEvents(opId, op);
    if(isEventActivityEvent(op->arg1)){
        edgeAdded2 = checkAndUpdateComponentState(opId, op);
        shouldAbort = !edgeAdded2 && !edgeAdded1;
    }else{
        shouldAbort = !edgeAdded1;
    }

    if(shouldAbort){
        LOGE("ABC: Trigger-Lifecycle event seen for component %d and state %d without a "
             "corresponding enable event or mismatch in activity state machine during processing."
             " Aborting processing.", op->arg2->id, op->arg1);
        gDvm.isRunABC = false;
    }

    return shouldAbort;
}

bool processInstanceIntentMap(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
    op->asyncId = threadBK->curAsyncId;
    bool shouldAbort = false;
 
    shouldAbort = abcMapInstanceWithIntentId(op->arg2->id, op->arg1);

    return shouldAbort;
}

//Service events are completely managed with TRIGGERs with supporting maps and state machines.
//Check AbcModel.cpp for its corresponding functions
bool processTriggerServiceLifecycleOperation(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
    op->asyncId = threadBK->curAsyncId;

    //stats needed when reporting races
    abcEventTriggerCount++;
    if(op->asyncId != -1 && op->arg1 != ABC_REQUEST_START_SERVICE && op->arg1 != ABC_REQUEST_BIND_SERVICE
           && op->arg1 != ABC_REQUEST_STOP_SERVICE && op->arg1 != ABC_REQUEST_UNBIND_SERVICE){
        AbcAsync* async = abcAsyncMap.find(op->asyncId)->second;
        async->recentTriggerOpId = opId;
        /*for events post is from native thread and hence this info is not useful
         *as far as possible consider only cross posts due to app code. Also for
         *events connections due to enable-trigger should be of importance and
         *lack of that leading to race should be reflected (categorizing it as
         *co-enabled race or unknown category race)
         */
        async->recentCrossPostAsync = -1;
    }

    bool serviceUpdated = checkAndUpdateServiceState(opId, op);
    bool shouldAbort = !serviceUpdated;
   
    return shouldAbort;
}

void processRegisterBroadcastReceiver(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
    op->asyncId = threadBK->curAsyncId;
    std::string action(op->arg5);
    std::map<std::string, std::pair<int,int> >::iterator it = abcRegisteredReceiversMap.find(action);
    if(it == abcRegisteredReceiversMap.end()){
        abcRegisteredReceiversMap.insert(std::make_pair(action, std::make_pair(opId,-1)));
    }else{
        it->second.first = opId;
    }
}

bool processTriggerBroadcastReceiver(int opId, AbcOp* op, AbcThreadBookKeep* threadBK){
    op->asyncId = threadBK->curAsyncId;
    bool shouldAbort = false;
 
    //stats needed when reporting races
    abcEventTriggerCount++;
    if(op->asyncId != -1 && op->arg1 == ABC_TRIGGER_ONRECIEVE){
        AbcAsync* async = abcAsyncMap.find(op->asyncId)->second;
        async->recentTriggerOpId = opId;
        /*for events post is from native thread and hence this info is not useful
         *as far as possible consider only cross posts due to app code. Also for
         *events connections due to enable-trigger should be of importance and
         *lack of that leading to race should be reflected (categorizing it as
         *co-enabled race or unknown category race)
         */
        async->recentCrossPostAsync = -1;
    }

    bool success = checkAndUpdateBroadcastState(opId, op);
    shouldAbort = !success;

    return shouldAbort;
}


//Graph closure and race detection related methods

void checkAndAdd_PO_edge(int opId, AbcOp* op, int prevOpId){
    if(prevOpId != -1){
    std::map<int, AbcOp*>::iterator prevOpIt = abcTrace.find(prevOpId);
    if(prevOpIt != abcTrace.end()){
        AbcOp* prevOp = prevOpIt->second;
        if(!(op->opType == ABC_THREADINIT || op->opType == ABC_CALL || 
             op->opType == ABC_NATIVE_ENTRY ||
             (op->opType == ABC_POST && prevOp->opType == ABC_RET))){
        
            addEdgeToHBGraph(prevOpId, opId);
        }
    }else{
        LOGE("ABC-MISSING: entry for operation %d missing in abcTrace", prevOpId);
    }
    }
}

bool isFirstPostAncestorOfSecond(int post1, int post2, AbcAsync* async2){
    bool isAncestor = false;
    AbcAsync* temp = async2;
    while(temp->parentAsyncId != -1){
        std::map<int, AbcAsync*>::iterator iter = abcAsyncMap.find(temp->parentAsyncId);
        if(iter != abcAsyncMap.end()){
            temp = iter->second;
        }else{
            LOGE("ABC-MISSING: async block missing entry");
            break;
        }

        if(temp->postId == post1){
            isAncestor = true;
            break;
        }
    }
 
    return isAncestor;
}

bool isDelayedAsyncFifo(int destPostAsyncId, AbcAsync* srcPostAsync, AbcAsync* destPostAsync){
    bool shouldAddFifo = false;
        if(destPostAsync->parentAsyncId != -1){
            std::map<int, AbcAsync*>::iterator parentIter = abcAsyncMap.find(destPostAsync->parentAsyncId);
            if(parentIter != abcAsyncMap.end()){
                if(adjGraph[srcPostAsync->retId - 1][parentIter->second->callId -1] == true){
                    shouldAddFifo = true;
                }
            }else{
                LOGE("ABC-MISSING: missing async block entry in asyncMap for id %d", destPostAsync->parentAsyncId);
            }
        }

    return shouldAddFifo;
}

bool checkAndAddAsyncFifoEdge(int o1, int o2, AbcOp* op1, AbcOp* op2){
    bool isFifo = false;
    
    //add fifo edge if both operations are POST and the destination of posts is the same thread
    //none of the posts should be such that their corresponding call is not present in trace
    //only when corresponding call is seen dest field of post gets field. 
    //if both are async blocks on the same thread 2nd async block's call could not have
    //happened before first ones ret. So just checking for dest tid not being -1 for both
    //is sufficient.
    if((op1->opType == ABC_POST && op2->opType == ABC_POST) &&
        (op1->arg3 != -1 && op2->arg3 != -1) && (op1->arg3 == op2->arg3)){
        std::map<int, AbcAsync*>::iterator iter1 = abcAsyncMap.find(op1->arg2->id);
        std::map<int, AbcAsync*>::iterator iter2 = abcAsyncMap.find(op2->arg2->id);
        if(iter1 != abcAsyncMap.end() && iter2 != abcAsyncMap.end()){
            AbcAsync* async1 = iter1->second;
            AbcAsync* async2 = iter2->second;
            //op1 is not a delayed message
            if(async1->retId != -1 && adjGraph[async1->retId - 1][async2->callId - 1] == false){    
                if(op1->arg4 == 0){
                    addEdgeToHBGraph(async1->retId, async2->callId); 
                }else if(op2->arg4 != 0){ //both op1 and op2 are delayed messages
                    if(op1->arg4 <= op2->arg4){
                        addEdgeToHBGraph(async1->retId, async2->callId);
                    }else if(isDelayedAsyncFifo(op2->arg2->id, async1, async2)){
                        addEdgeToHBGraph(async1->retId, async2->callId);
                    } 
                }else{  //only op1 is delayed message
                    if(isDelayedAsyncFifo(op2->arg2->id, async1, async2)){
                        addEdgeToHBGraph(async1->retId, async2->callId);
                    } 
                }
            }
            isFifo = true;
        }else{
            LOGE("ABC-MISSING: either of %d or %d messages do not have corresponding entry"
                 " in abcAsyncMap", op1->arg2->id, op2->arg2->id);
        }
    }

    return isFifo;
}

void checkAndAddSecondFifoEdge(int o1, int o2, AbcOp* op1, AbcOp* op2){
    //if both operations are POST and the destination of posts is the same thread
    if((op1->opType == ABC_CALL && op2->opType == ABC_CALL) &&
            (op1->tid == op2->tid)){
        std::map<int, AbcAsync*>::iterator iter1 = abcAsyncMap.find(op1->arg2->id);
        std::map<int, AbcAsync*>::iterator iter2 = abcAsyncMap.find(op2->arg2->id);
        if(iter1 != abcAsyncMap.end() && iter2 != abcAsyncMap.end()){
            AbcAsync* async1 = iter1->second;
            AbcAsync* async2 = iter2->second;
            if(adjGraph[async1->postId - 1][async2->postId - 1] == false){
                addEdgeToHBGraph(async1->postId, async2->postId);
            }
        }else{
            LOGE("ABC-MISSING: either of %d or %d messages do not have corresponding entry"
                 " in abcAsyncMap", op1->arg2->id, op2->arg2->id);
        }
    }
}

void checkAndAddRetToTriggerIfRetToEnableExists(int o1, int o2, AbcOp* op1, AbcOp* op2){
    //if edge exists from ret to enable then edge exists from ret to call of trigger
    if((op1->opType == ABC_RET && (op2->opType == ABC_ENABLE_EVENT || 
            op2->opType == ABC_ENABLE_LIFECYCLE || op2->opType == ABC_REGISTER_RECEIVER))){

        std::map<std::pair<u4, int>, std::pair<int,int> >::iterator it1;
        std::map<std::pair<int, int>, std::pair<int, int> >::iterator it2;
        std::map<std::string, std::pair<int,int> >::iterator it3;
 
        int triggerId = -1;
        switch(op2->opType){
            case ABC_ENABLE_EVENT: 
                 it1 =  abcEnabledEventMap.find(std::make_pair(op2->arg2->id, op2->arg1));
                 if(it1 != abcEnabledEventMap.end()){
                     triggerId = it1->second.second;
                 }
                 break;
            case ABC_ENABLE_LIFECYCLE:
                 it2 = abcEnabledLifecycleMap.find(std::make_pair(op2->arg2->id, op2->arg1));
                 if(it2 != abcEnabledLifecycleMap.end()){
                     triggerId = it2->second.second;
                 }
                 break;
            case ABC_REGISTER_RECEIVER:
                 std::string action(op2->arg5);
                 it3 = abcRegisteredReceiversMap.find(action);
                 if(it3 != abcRegisteredReceiversMap.end()){
                     triggerId = it3->second.second;
                 }
                 break;
        }

        
        if(triggerId != -1){
        std::map<int, AbcAsync*>::iterator iter = abcAsyncMap.find(abcTrace.find(triggerId)->second->asyncId);
        if(iter != abcAsyncMap.end()){
            AbcAsync* async = iter->second;
            if(adjGraph[o1 - 1][async->callId - 1] == false){
                addEdgeToHBGraph(o1, async->callId);
            }
        }else{
            LOGE("ABC-MISSING: either of %d or %d messages do not have corresponding entry"
                 " in abcAsyncMap", op1->arg2->id, op2->arg2->id);
        }
        }
    }
}

void checkAndAddAsyncNopreEdge(int o1, int o2, AbcOp* op1, AbcOp* op2){
    if(op2->opType == ABC_CALL && op1->tid == op2->tid &&
            op1->asyncId != -1){
        std::map<int, AbcAsync*>::iterator iter1 = abcAsyncMap.find(op1->asyncId);
        if(iter1 != abcAsyncMap.end()){
            AbcAsync* async1 = iter1->second;
            if(adjGraph[async1->retId - 1][o2 - 1] == false){
                addEdgeToHBGraph(async1->retId, o2);
            }
        }else{
            LOGE("ABC-MISSING: entry for asyncId %d missing in abcAsyncMap", op1->asyncId);
        }
    } 
}

//return true if o1 and o2 are ABC_TRIGGER_RECEIVER so that we need not check for other
//operator specific rules
/*sticky-register rule*/
bool checkAndAddStickyBroadcastRegisterEdge(int o1, int o2, AbcOp* op1, AbcOp* op2){
    bool isBothTriggerReceiver = false;
    if(op1->opType == ABC_TRIGGER_RECEIVER && op1->arg1 == ABC_REGISTER_RECEIVER &&
           op2->opType == ABC_TRIGGER_RECEIVER && op2->arg1 == ABC_REGISTER_RECEIVER){
        std::map<int, AbcSticky*>::iterator it1 = AbcRegisterOnReceiveMap.find(o1);
        std::map<int, AbcSticky*>::iterator it2 = AbcRegisterOnReceiveMap.find(o2);
        if(it1 != AbcRegisterOnReceiveMap.end() && it2 != AbcRegisterOnReceiveMap.end()){
            /*if registerReceiver1 < registerReceiver2 such that both are registered
             *when corresponding action stickyintents are around or atleast the first
             *one is sticky then onReceive1 < onReceive2 AND post(onReceive1) < post(onReceive2)
             */
             if(it1->second->isSticky && it2->second->isSticky){
                 assert(it1->second->op->opId < it2->second->op->opId);
                 addEdgeToHBGraph(it1->second->op->opId, it2->second->op->opId);
                 AbcAsync* async1 = getAsyncBlockFromId(it1->second->op->opPtr->asyncId);
                 AbcAsync* async2 = getAsyncBlockFromId(it2->second->op->opPtr->asyncId);
                 addEdgeToHBGraph(async1->postId, async2->postId);
             }
        }
        
        isBothTriggerReceiver = true;
    }

    return isBothTriggerReceiver;
}

//return true if o1 and o2 are ABC_TRIGGER_RECEIVER so that we need not check for other
//operator specific rules
/*sticky-register rule*/
bool checkAndAddSendBroadcastEdge(int o1, int o2, AbcOp* op1, AbcOp* op2){
    bool isBothTriggerReceiver = false;
    if(op1->opType == ABC_TRIGGER_RECEIVER && (op1->arg1 == ABC_SEND_BROADCAST ||
           op1->arg1 == ABC_SEND_STICKY_BROADCAST) &&
           op2->opType == ABC_TRIGGER_RECEIVER && (op2->arg1 == ABC_SEND_BROADCAST ||
           op2->arg1 == ABC_SEND_STICKY_BROADCAST)){
    
        std::map<int, std::list<AbcOpWithId*> >::iterator it1 = AbcSendBroadcastOnReceiveMap.find(o1);
        std::map<int, std::list<AbcOpWithId*> >::iterator it2 = AbcSendBroadcastOnReceiveMap.find(o2);
        if(it1 != AbcSendBroadcastOnReceiveMap.end() && it2 != AbcSendBroadcastOnReceiveMap.end()){
            for(std::list<AbcOpWithId*>::iterator i1 = it1->second.begin(); i1 != it1->second.end(); i1++){
                AbcAsync* async1 = getAsyncBlockFromId((*i1)->opPtr->asyncId);
                for(std::list<AbcOpWithId*>::iterator i2 = it2->second.begin(); i2 != it2->second.end(); i2++){
                    AbcAsync* async2 = getAsyncBlockFromId((*i2)->opPtr->asyncId);
                    addEdgeToHBGraph((*i1)->opId, (*i2)->opId);
                    addEdgeToHBGraph(async1->postId, async2->postId);
                }
            }
        }

        isBothTriggerReceiver = true;
    }

    return isBothTriggerReceiver;
}

bool checkAndAddCondTransEdge(int o1, int o2, int o3, AbcOp* op1, AbcOp* op2, AbcOp* op3){
    bool isCondTrans = false;
    //check and add edge from op1 to op3
//    LOGE("check cond trans between %d, %d, %d", o1, o2, o3);
    if(op1->asyncId == -1 || op3->asyncId == -1 ||
        (op1->tid != op3->tid)){
        isCondTrans = true;
    }else{
        std::map<int, AbcAsync*>::iterator asyncIt1 = abcAsyncMap.find(op1->asyncId);
        std::map<int, AbcAsync*>::iterator asyncIt3 = abcAsyncMap.find(op3->asyncId);
        if(asyncIt1 != abcAsyncMap.end() && asyncIt3 != abcAsyncMap.end()){

        AbcAsync* async1 = asyncIt1->second;
        AbcAsync* async3 = asyncIt3->second;
        if(async1->callId != async3->callId &&
            adjGraph[async1->retId - 1][async3->callId - 1] == false){
            //conditional transitivity base case
            int op2AsyncId;
            if(op2->opType == ABC_POST && op3->opType == ABC_CALL &&
                    ((op2AsyncId = op2->arg2->id) == op3->asyncId)){
                isCondTrans = true;
            }
        }else{
            isCondTrans = true;
        }

        }else{
            LOGE("ABC-MISSING: one among the async ids %d and %d do not have an entry in abcAsyncMap", op1->asyncId, op3->asyncId);
        }
    }

    if(isCondTrans){
        if(adjGraph[o1 - 1][o3 - 1] == false){
            addEdgeToHBGraph(o1, o3);
        }else{
            isCondTrans = false;
        }
    }

    //return true only if an edge got added
    return isCondTrans;
}

void computeClosureOfHbGraph(){
    WorklistElem* edge;
    while((edge = getNextEdgeFromWorklist()) != NULL){
    //    LOGE("ABC: procesing edge %d and %d", edge->src, edge->dest);
        std::map<int, AbcOp*>::iterator opIt1 = abcTrace.find(edge->src);
        std::map<int, AbcOp*>::iterator opIt2 = abcTrace.find(edge->dest);
        if(opIt1 != abcTrace.end() && opIt2 != abcTrace.end()){
         
        AbcOp* src = opIt1->second;
        AbcOp* dest = opIt2->second;
        //ST-ASYNC-FIFO
        bool isFifo = checkAndAddAsyncFifoEdge(edge->src, edge->dest, src, dest);
        
        bool isBothRegisterReceiver = false;
        if(!isFifo){
            isBothRegisterReceiver = checkAndAddStickyBroadcastRegisterEdge(edge->src, edge->dest, src, dest);
        }

        bool isBothSendBroadcasts = false;
        if(!isFifo && !isBothRegisterReceiver){
            isBothSendBroadcasts = checkAndAddSendBroadcastEdge(edge->src, edge->dest, src, dest);
        }

        //ST-ASYNC-NOPRE
        if(!isFifo && !isBothRegisterReceiver && !isBothSendBroadcasts){
            checkAndAddAsyncNopreEdge(edge->src, edge->dest, src, dest);
        }
       
        //COND-TRANS
        bool isNewEdgeAdded = false;
 
        std::map<int, std::pair<Destination*, Source*> >::iterator 
                srcIter = adjMap.find(edge->src);
        if(srcIter != adjMap.end()){

        Source* srcList = srcIter->second.second;
        std::map<int, AbcOp*>::iterator inter;
        while(srcList != NULL){
            //check for con-trans condition
            inter = abcTrace.find(srcList->src);
            if(inter != abcTrace.end()){
            bool condTransEdgeAdded = checkAndAddCondTransEdge(srcList->src, 
                    edge->src, edge->dest, inter->second, src, dest); 

            if(condTransEdgeAdded){
               isNewEdgeAdded = true; 
            }

            }else{
                LOGE("ABC-MISSING: an operation %d missing entry in abcTrace", srcList->src);
            }
            srcList = srcList->prev;
        }

        if(isNewEdgeAdded){
            std::map<int, std::pair<Destination*, Source*> >::iterator
                    destIter = adjMap.find(edge->dest);
            if(destIter != adjMap.end()){

            Destination* destList = destIter->second.first;
            while(destList != NULL){
                inter = abcTrace.find(destList->dest);
                if(inter != abcTrace.end()){
                    checkAndAddCondTransEdge(edge->src, edge->dest, destList->dest, src, dest, inter->second);
                }else{
                    LOGE("ABC-MISSING: an operation %d missing entry in abcTrace", destList->dest);
                }
                destList = destList->prev;
            }

            }else{
                LOGE("ABC-MISSING: source-dest map entry missing for node %d", edge->dest);
            }
        }
        }else{
            LOGE("ABC-MISSING: source-dest map entry missing for node %d", edge->src);
        }        

        }else{
            LOGE("ABC-MISSING: either of operations %d or %d do not have an entry in abcTrace", edge->src, edge->dest);
        }    
        free(edge);
    }
}

void removeRWfromAccessSet(int rwId){
    std::map<int, AbcRWAccess*>::iterator it = abcRWAccesses.find(rwId);
    if(it != abcRWAccesses.end()){
        int accessId = it->second->accessId;
        std::map<int, std::pair<int, std::list<int> > >::iterator absIt = abcRWAbstractionMap.find(accessId);
        if(absIt != abcRWAbstractionMap.end()){
            std::list<int> rwList = absIt->second.second;
            rwList.remove(rwId);
        }else{
            LOGE("ABC-MISSING: missing entry for accesssId %d in abcRWAbstractionMap", accessId);
        }
    }else{
        LOGE("ABC-MISSING: abcRWAccesses entry misssing for rwId %d", rwId);
    }
}

int getAsyncBlockOfReadWriteOp(int rwId){
    int accessId = abcRWAccesses.find(rwId)->second->accessId;
    
    int opId = abcRWAbstractionMap.find(accessId)->second.first;

    int asyncId = abcTrace.find(opId)->second->asyncId;
    return asyncId;
}

void removeRedundantReadWrites(){
    //iterate over Object access map
    LOGE("ABC:cleanup abcObjectAccessMap");
    for(std::map<std::pair<Object*, u4>, std::pair<std::set<int>, std::set<int> > >::iterator it
            = abcObjectAccessMap.begin(); it != abcObjectAccessMap.end(); ){
        if(it->second.second.size() == 0){

            std::set<int>::iterator setIt = it->second.first.begin();
            //take some race detection stats before doing anything
            std::map<int, AbcRWAccess*>::iterator accIter = abcRWAccesses.find(*setIt);
            if(accIter != abcRWAccesses.end()){
                std::pair<const char*, u4> classField = std::make_pair(accIter->second->clazz, accIter->second->fieldIdx);
                if(fieldSet.find(classField) == fieldSet.end()){
                    fieldSet.insert(classField);
                }
            }

            //perform cleanup

            for(; setIt != it->second.first.end(); ++setIt){
                removeRWfromAccessSet(*setIt);
            }
            abcObjectAccessMap.erase(it++);
        }else{
            /*first iterate over write set and identify if accesses to this object come from
             *different async blocks or threads
             */
            std::set<int>::iterator setIt = it->second.second.begin();
            std::map<int, AbcRWAccess*>::iterator rwIter;
            rwIter = abcRWAccesses.find(*setIt);
            if(rwIter != abcRWAccesses.end()){
            
            //take some race detection stats before doing anything
            std::pair<const char*, u4> classField = std::make_pair(rwIter->second->clazz, rwIter->second->fieldIdx);
            if(fieldSet.find(classField) == fieldSet.end()){
                fieldSet.insert(classField);
            }

            //perform cleanup
            int tid = rwIter->second->tid;
            int asyncId = getAsyncBlockOfReadWriteOp(*setIt);
            ++setIt;
            bool shouldRemoveObject = true;
            for( ; setIt != it->second.second.end(); ++setIt){
                rwIter = abcRWAccesses.find(*setIt);
                if(rwIter != abcRWAccesses.end()){

                int tmpTid = rwIter->second->tid;
                int tmpAsyncId = getAsyncBlockOfReadWriteOp(*setIt);
                if(tmpTid == tid && tmpAsyncId == asyncId){
                    continue;
                }else{
                    shouldRemoveObject = false;
                    break;
                }

                }else{
                    LOGE("ABC-MISSING: abcRWAccesses object access entry misssing for rwId %d", *setIt);
                    shouldRemoveObject = false;
                    break;
                }  
            }

            if(shouldRemoveObject){
                //iterate over read set and check if any interesting reads are present
                for(setIt = it->second.first.begin(); setIt != it->second.first.end(); ++setIt){
                   rwIter = abcRWAccesses.find(*setIt);
                   if(rwIter != abcRWAccesses.end()){

                   int tmpTid = rwIter->second->tid;
                   int tmpAsyncId = getAsyncBlockOfReadWriteOp(*setIt);
                   if(tmpTid == tid && tmpAsyncId == asyncId){
                       continue;
                   }else{
                       shouldRemoveObject = false;
                       break;
                   }

                   }else{
                       LOGE("ABC-MISSING: abcRWAccesses object access entry misssing for rwId %d", *setIt);
                       shouldRemoveObject = false;
                       break;
                   } 
                }
            }

            if(shouldRemoveObject){
                for(std::set<int>::iterator setIt = it->second.first.begin();
                    setIt != it->second.first.end(); ++setIt){
                    removeRWfromAccessSet(*setIt);
                }
                for(std::set<int>::iterator setIt = it->second.second.begin();
                    setIt != it->second.second.end(); ++setIt){
                    removeRWfromAccessSet(*setIt);
                }
                abcObjectAccessMap.erase(it++);
            }else{
                ++it;
            }
        
        }else{
            LOGE("ABC-MISSING: abcRWAccesses object access entry misssing for rwId %d", *setIt);
        }
        } 
    }


    //iterate over Database access map
    LOGE("ABC:cleanup abcDatabaseAccessMap");
    for(std::map<std::string, std::pair<std::set<int>, std::set<int> > >::iterator it
            = abcDatabaseAccessMap.begin(); it != abcDatabaseAccessMap.end(); ){
        if(it->second.second.size() == 0){
            //needed for race detection stats collection
            dbFieldSet.insert(it->first);
            
            for(std::set<int>::iterator setIt = it->second.first.begin();
                    setIt != it->second.first.end(); ++setIt){
                removeRWfromAccessSet(*setIt);
            }
            abcDatabaseAccessMap.erase(it++);
        }else{
            /*first iterate over write set and identify if accesses to this object come from
             *different async blocks or threads
             */

            //needed for race detection stats collection
            dbFieldSet.insert(it->first);

            std::set<int>::iterator setIt = it->second.second.begin();
            std::map<int, AbcRWAccess*>::iterator rwIter;
            rwIter = abcRWAccesses.find(*setIt);
            if(rwIter != abcRWAccesses.end()){

            int tid = rwIter->second->tid;
            int asyncId = getAsyncBlockOfReadWriteOp(*setIt);
            
            ++setIt;
            bool shouldRemoveObject = true;
            for( ; setIt != it->second.second.end(); ++setIt){
                rwIter = abcRWAccesses.find(*setIt);
                if(rwIter != abcRWAccesses.end()){

                int tmpTid = rwIter->second->tid;
                int tmpAsyncId = getAsyncBlockOfReadWriteOp(*setIt);
                if(tmpTid == tid && tmpAsyncId == asyncId){
                    continue;
                }else{
                    shouldRemoveObject = false;
                    break;
                }
    
                }else{
                    LOGE("ABC-MISSING: abcRWAccesses database access entry misssing for rwId %d", *setIt);
                    shouldRemoveObject = false;
                    break;
                }
            }

            if(shouldRemoveObject){
                //iterate over read set and check if any interesting reads are present
                for(setIt = it->second.first.begin(); setIt != it->second.first.end(); ++setIt){
                   rwIter = abcRWAccesses.find(*setIt);
                   if(rwIter != abcRWAccesses.end()){

                   int tmpTid = rwIter->second->tid;
                   int tmpAsyncId = getAsyncBlockOfReadWriteOp(*setIt);
                   if(tmpTid == tid && tmpAsyncId == asyncId){
                       continue;
                   }else{
                       shouldRemoveObject = false;
                       break;
                   } 

                   }else{
                       LOGE("ABC-MISSING: abcRWAccesses database access entry misssing for rwId %d", *setIt);
                       shouldRemoveObject = false;
                       break;
                   }
                }
            }

            if(shouldRemoveObject){
                for(std::set<int>::iterator setIt = it->second.first.begin();
                    setIt != it->second.first.end(); ++setIt){
                    removeRWfromAccessSet(*setIt);
                }
                for(std::set<int>::iterator setIt = it->second.second.begin();
                    setIt != it->second.second.end(); ++setIt){
                    removeRWfromAccessSet(*setIt);
                }
                abcDatabaseAccessMap.erase(it++);
            }else{
                ++it;
            }

        }else{
            LOGE("ABC-MISSING: abcRWAccesses database access entry misssing for rwId %d", *setIt);
        }
        } 
    }


    //iterate over Static access map
    LOGE("ABC:cleanup abcStaticAccessMap");
    for(std::map<std::pair<const char*, u4>, std::pair<std::set<int>, std::set<int> > >::iterator it
            = abcStaticAccessMap.begin(); it != abcStaticAccessMap.end(); ){
        if(it->second.second.size() == 0){
            std::set<int>::iterator setIt = it->second.first.begin();
            //take some race detection stats before doing anything
            std::map<int, AbcRWAccess*>::iterator accIter = abcRWAccesses.find(*setIt);
            if(accIter != abcRWAccesses.end()){
                std::pair<const char*, u4> classField = std::make_pair(accIter->second->clazz, accIter->second->fieldIdx);
                if(fieldSet.find(classField) == fieldSet.end()){
                    fieldSet.insert(classField);
                }
            }
            
            //perform cleanup
            for(; setIt != it->second.first.end(); ++setIt){
                removeRWfromAccessSet(*setIt);
            }
            abcStaticAccessMap.erase(it++);
        }else{
            /*first iterate over write set and identify if accesses to this object come from
             *different async blocks or threads
             */
            std::set<int>::iterator setIt = it->second.second.begin();
            std::map<int, AbcRWAccess*>::iterator rwIter;
            rwIter = abcRWAccesses.find(*setIt);
            if(rwIter != abcRWAccesses.end()){

            //take some race detection stats before doing anything
            std::pair<const char*, u4> classField = std::make_pair(rwIter->second->clazz, rwIter->second->fieldIdx);
            if(fieldSet.find(classField) == fieldSet.end()){
                fieldSet.insert(classField);
            }

            //perform cleanup
            int tid = rwIter->second->tid;
            int asyncId = getAsyncBlockOfReadWriteOp(*setIt);
            
            ++setIt;
            bool shouldRemoveObject = true;
            for( ; setIt != it->second.second.end(); ++setIt){
                rwIter = abcRWAccesses.find(*setIt);
                if(rwIter != abcRWAccesses.end()){
                
                int tmpTid = rwIter->second->tid;
                int tmpAsyncId = getAsyncBlockOfReadWriteOp(*setIt);
                if(tmpTid == tid && tmpAsyncId == asyncId){
                    continue;
                }else{
                    shouldRemoveObject = false;
                    break;
                }

                }else{
                    LOGE("ABC-MISSING: abcRWAccesses static access entry misssing for rwId %d", *setIt);
                    shouldRemoveObject = false;
                    break;
                }
            }

            if(shouldRemoveObject){
                //iterate over read set and check if any interesting reads are present
                for(setIt = it->second.first.begin(); setIt != it->second.first.end(); ++setIt){
                   rwIter = abcRWAccesses.find(*setIt);
                   if(rwIter != abcRWAccesses.end()){

                   int tmpTid = rwIter->second->tid;
                   int tmpAsyncId = getAsyncBlockOfReadWriteOp(*setIt);
                   if(tmpTid == tid && tmpAsyncId == asyncId){
                       continue;
                   }else{
                       shouldRemoveObject = false;
                       break;
                   } 

                   }else{
                       LOGE("ABC-MISSING: abcRWAccesses static accesses entry misssing for rwId %d", *setIt);
                       shouldRemoveObject = false;
                       break;
                   }
                }
            }

            if(shouldRemoveObject){
                for(std::set<int>::iterator setIt = it->second.first.begin();
                    setIt != it->second.first.end(); ++setIt){
                    removeRWfromAccessSet(*setIt);
                }
                for(std::set<int>::iterator setIt = it->second.second.begin();
                    setIt != it->second.second.end(); ++setIt){
                    removeRWfromAccessSet(*setIt);
                }
                abcStaticAccessMap.erase(it++);
            }else{
                ++it;
            }
        }else{
            LOGE("ABC-MISSING: abcRWAccesses static accesses entry misssing for rwId %d", *setIt);
        }
        } 
    }
}

int getOpIdOfReadWrite(int rwId){
    int opId = -1;
    std::map<int, AbcRWAccess*>::iterator rwIter = abcRWAccesses.find(rwId);
    if(rwIter != abcRWAccesses.end()){
        int accessId = rwIter->second->accessId;
        std::map<int, std::pair<int, std::list<int> > >::iterator
                absIter = abcRWAbstractionMap.find(accessId);
        if(absIter != abcRWAbstractionMap.end()){
            opId = absIter->second.first;
        }else{
            LOGE("ABC-MISSING: entry for accessId %d missing in abcRWAbstractionMap", accessId);
        }
    }else{
        LOGE("ABC-MISSING: entry for rwId %d missing in abcRWAccesses", rwId);
    }
    return opId;
}

//o1 and o2 coresponds to opIds of corresponding accessIds
//we do not report races on database as we do not capture it accurately
void collectStatsOnTheRace(int rwId1, int rwId2, AbcRWAccess* acc1, AbcRWAccess* acc2, int o1, int o2){
    bool inserted = false;
    std::pair<std::set<std::pair<const char*, u4> >::iterator, bool> resultClass; 
    std::pair<std::set<std::pair<Object*, u4> >::iterator, bool> resultObj; 
    std::pair<std::set<std::string>::iterator, bool> resultDb;
    //check if the race is a multithreaded race
    if(acc1->tid != acc2->tid){
        //set semantics ensures uniqueness of elements...a field wont be added more than once
        if(acc1->clazz != NULL){
            std::pair<const char*, u4> classField = std::make_pair(acc1->clazz, acc1->fieldIdx);
            multiThreadRaces.insert(classField);
            if(acc1->obj != NULL){
                std::pair<Object*, u4> objField = std::make_pair(acc1->obj, acc1->fieldIdx);
                resultObj = multiThreadRacesObj.insert(objField);
                inserted = resultObj.second;
            }else{
                resultClass = multiThreadRacesClass.insert(classField);
                inserted = resultClass.second;
            }
        }/*else{
            std::string dbPath(acc1->dbPath);
            resultDb = dbMultiThreadRaces.insert(dbPath);
            inserted = resultDb.second;
        } */
    }else{

    //categorize the async race..it may belong to multiple categories
        std::map<int, AbcOp*>::iterator opIter1 = abcTrace.find(o1);
        std::map<int, AbcOp*>::iterator opIter2 = abcTrace.find(o2);
        if(opIter1 != abcTrace.end() && opIter2 != abcTrace.end()){
            AbcOp* op1 = opIter1->second;
            AbcOp* op2 = opIter2->second;
            if(op1->asyncId != -1 && op2->asyncId != -1){

                std::map<int, AbcAsync*>::iterator asyncIter1 = abcAsyncMap.find(op1->asyncId); 
                std::map<int, AbcAsync*>::iterator asyncIter2 = abcAsyncMap.find(op2->asyncId); 
                if(asyncIter1 != abcAsyncMap.end() && asyncIter2 != abcAsyncMap.end()){
                    AbcAsync* async1 = asyncIter1->second;
                    AbcAsync* async2 = asyncIter2->second;
                    //if one of the read/write has no corresponding trigger-event then dont consider 
                    //it for co-enabled events categorization
                    if(async1->recentTriggerOpId != -1 && async2->recentTriggerOpId != -1 && 
                        (async1->recentTriggerOpId != async2->recentTriggerOpId)){
                        if(isHbEdge(async1->recentTriggerOpId, async2->recentTriggerOpId) ||
                                isHbEdge(async2->recentTriggerOpId, async1->recentTriggerOpId)){

                            //firstly, add this to the list of races
                            if(acc1->clazz != NULL){
                                std::pair<const char*, u4> classField = std::make_pair(acc1->clazz, acc1->fieldIdx);
                                raceyFieldSet.insert(classField);
                            }else{
                                std::string dbPath(acc1->dbPath);
                                dbRaceyFieldSet.insert(dbPath);
                            }

                            bool raceCategoryDetected = false;
                            //check if delayed post is a reason
                            if(async1->recentDelayAsync != async2->recentDelayAsync && 
                                   (async1->recentDelayAsync != -1 || async2->recentDelayAsync != -1)){
                                raceCategoryDetected = true;

                                if(acc1->clazz != NULL){
                                    std::pair<const char*, u4> classField = std::make_pair(acc1->clazz, acc1->fieldIdx);
                                    delayPostRaces.insert(classField);
                                    if(acc1->obj != NULL){
                                        std::pair<Object*, u4> objField = std::make_pair(acc1->obj, acc1->fieldIdx);
                                        resultObj = delayPostRacesObj.insert(objField);
                                        inserted = resultObj.second;
                                    }else{
                                        resultClass = delayPostRacesClass.insert(classField);
                                        inserted = resultClass.second;
                                    }
                                }/*else{
                                    std::string dbPath(acc1->dbPath);
                                    resultDb = dbDelayPostRaces.insert(dbPath);
                                    inserted = resultDb.second;
                                }*/
                            }                             

                            //check if cross thread post is a reason
                            if(!inserted && async1->recentCrossPostAsync != async2->recentCrossPostAsync &&
                                   (async1->recentCrossPostAsync != -1 || async2->recentCrossPostAsync != -1)){
                                raceCategoryDetected = true;
                                if(acc1->clazz != NULL){
                                    std::pair<const char*, u4> classField = std::make_pair(acc1->clazz, acc1->fieldIdx);
                                    crossPostRaces.insert(classField);
                                    if(acc1->obj != NULL){
                                        std::pair<Object*, u4> objField = std::make_pair(acc1->obj, acc1->fieldIdx);
                                        resultObj = crossPostRacesObj.insert(objField);
                                        inserted = resultObj.second;
                                    }else{
                                        resultClass = crossPostRacesClass.insert(classField);
                                        inserted = resultClass.second;
                                    }
                                }/*else{
                                    std::string dbPath(acc1->dbPath);
                                    resultDb = dbCrossPostRaces.insert(dbPath);
                                    inserted = resultDb.second;
                                }*/
                            }   


                            if(!raceCategoryDetected){
                                if(acc1->clazz != NULL){
                                    std::pair<const char*, u4> classField = std::make_pair(acc1->clazz, acc1->fieldIdx);
                                    uncategorizedRaces.insert(classField);
                                    if(acc1->obj != NULL){
                                        std::pair<Object*, u4> objField = std::make_pair(acc1->obj, acc1->fieldIdx);
                                        resultObj = uncategorizedRacesObj.insert(objField);
                                        inserted = resultObj.second;
                                    }else{
                                        resultClass = uncategorizedRacesClass.insert(classField);
                                        inserted = resultClass.second;
                                    }

                                }/*else{
                                    std::string dbPath(acc1->dbPath);
                                    resultDb = dbUncategorizedRaces.insert(dbPath);
                                    inserted = resultDb.second;
                                }*/
                            }

                        }else{
                            //race is due to co-enabled triggers
                            
                            //check if both the co-enabled events are UI events and filter them
                            //firstly, add this to the list of races
                            if(acc1->clazz != NULL){
                                std::pair<const char*, u4> classField = std::make_pair(acc1->clazz, acc1->fieldIdx);
                                raceyFieldSet.insert(classField);
                            }else{
                                std::string dbPath(acc1->dbPath);
                                dbRaceyFieldSet.insert(dbPath);
                            }

                            //check if class is a UI class or not and classify appropriately
                            std::string clazz(acc1->clazz);
                            if(UiWidgetSet.find(clazz) != UiWidgetSet.end()){
                                std::pair<const char*, u4> classField = std::make_pair(acc1->clazz, acc1->fieldIdx);
                               /* coEnabledEventUiRaces.insert(std::make_pair(classField, std::make_pair(
                                        async1->recentTriggerOpId, async2->recentTriggerOpId)));   */
                                if(acc1->obj != NULL){
                                    std::pair<Object*, u4> objField = std::make_pair(acc1->obj, acc1->fieldIdx);
                                    std::pair<std::map<std::pair<Object*, u4>, std::pair<int,int> >::iterator, bool> resUiObj
                                        = coEnabledEventUiRacesObj.insert(std::make_pair(objField, std::make_pair(
                                        async1->recentTriggerOpId, async2->recentTriggerOpId)));
                                    inserted = resUiObj.second;
                                }else{
                                    std::pair<std::map<std::pair<const char*, u4>, std::pair<int,int> >::iterator, bool> resUiClass
                                        = coEnabledEventUiRacesClass.insert(std::make_pair(classField, std::make_pair(
                                        async1->recentTriggerOpId, async2->recentTriggerOpId)));
                                    inserted = resUiClass.second;
                                }
                            }else{
                                if(acc1->clazz != NULL){
                                    std::pair<const char*, u4> classField = std::make_pair(acc1->clazz, acc1->fieldIdx);
                                   /* coEnabledEventNonUiRaces.insert(std::make_pair(classField, std::make_pair(
                                        async1->recentTriggerOpId, async2->recentTriggerOpId)));*/
                                    if(acc1->obj != NULL){
                                        std::pair<Object*, u4> objField = std::make_pair(acc1->obj, acc1->fieldIdx);
                                        std::pair<std::map<std::pair<Object*, u4>, std::pair<int,int> >::iterator, bool> resUiObj
                                            = coEnabledEventNonUiRacesObj.insert(std::make_pair(objField, std::make_pair(
                                            async1->recentTriggerOpId, async2->recentTriggerOpId)));
                                        inserted = resUiObj.second;
                                    }else{
                                        std::pair<std::map<std::pair<const char*, u4>, std::pair<int,int> >::iterator, bool> resUiClass
                                            = coEnabledEventNonUiRacesClass.insert(std::make_pair(classField, std::make_pair(
                                            async1->recentTriggerOpId, async2->recentTriggerOpId)));
                                        inserted = resUiClass.second;
                                    }

                                }/*else{
                                    std::string dbPath(acc1->dbPath);
                                    std::pair<std::map<std::string, std::pair<int,int> >::iterator, bool> resNonUiDb 
                                        = dbCoEnabledEventRaces.insert(std::make_pair(dbPath, std::make_pair(
                                        async1->recentTriggerOpId, async2->recentTriggerOpId))); 
                                    inserted = resNonUiDb.second;
                                }*/
                            }
                           
                    /*        }//end of filter  */
                        }
                    }else{ //cannot find the corresponding triggers for one of the accesses. So test for other categpries

                            //firstly, add this to the list of races
                            if(acc1->clazz != NULL){
                                std::pair<const char*, u4> classField = std::make_pair(acc1->clazz, acc1->fieldIdx);
                                raceyFieldSet.insert(classField);
                            }else{
                                std::string dbPath(acc1->dbPath);
                                dbRaceyFieldSet.insert(dbPath);
                            }
                            
                            bool raceCategoryDetected = false;
                            //check if delayed post is a reason
                            if(async1->recentDelayAsync != async2->recentDelayAsync &&
                                   (async1->recentDelayAsync != -1 || async2->recentDelayAsync != -1)){
                                raceCategoryDetected = true;

                                if(acc1->clazz != NULL){
                                    std::pair<const char*, u4> classField = std::make_pair(acc1->clazz, acc1->fieldIdx);
                                    delayPostRaces.insert(classField);
                                    if(acc1->obj != NULL){
                                        std::pair<Object*, u4> objField = std::make_pair(acc1->obj, acc1->fieldIdx);
                                        resultObj = delayPostRacesObj.insert(objField);
                                        inserted = resultObj.second;
                                    }else{
                                        resultClass = delayPostRacesClass.insert(classField);
                                        inserted = resultClass.second;
                                    }

                                }/*else{
                                    std::string dbPath(acc1->dbPath);
                                    resultDb = dbDelayPostRaces.insert(dbPath);
                                    inserted = resultDb.second;
                                }*/
                            }

                            //check if cross thread post is a reason
                            if(!inserted && async1->recentCrossPostAsync != async2->recentCrossPostAsync &&
                                   (async1->recentCrossPostAsync != -1 || async2->recentCrossPostAsync != -1)){
                                raceCategoryDetected = true;
                                if(acc1->clazz != NULL){
                                    std::pair<const char*, u4> classField = std::make_pair(acc1->clazz, acc1->fieldIdx);
                                    crossPostRaces.insert(classField);
                                    if(acc1->obj != NULL){
                                        std::pair<Object*, u4> objField = std::make_pair(acc1->obj, acc1->fieldIdx);
                                        resultObj = crossPostRacesObj.insert(objField);
                                        inserted = resultObj.second;
                                    }else{
                                        resultClass = crossPostRacesClass.insert(classField);
                                        inserted = resultClass.second;
                                    }

                                }/*else{
                                    std::string dbPath(acc1->dbPath);
                                    resultDb = dbCrossPostRaces.insert(dbPath);
                                    inserted = resultDb.second;
                                }*/
                            }

                            if(!raceCategoryDetected){
                                if(acc1->clazz != NULL){
                                    std::pair<const char*, u4> classField = std::make_pair(acc1->clazz, acc1->fieldIdx);
                                    uncategorizedRaces.insert(classField);
                                    if(acc1->obj != NULL){
                                        std::pair<Object*, u4> objField = std::make_pair(acc1->obj, acc1->fieldIdx);
                                        resultObj = uncategorizedRacesObj.insert(objField);
                                        inserted = resultObj.second;
                                    }else{
                                        resultClass = uncategorizedRacesClass.insert(classField);
                                        inserted = resultClass.second;
                                    }

                                }/*else{
                                    std::string dbPath(acc1->dbPath);
                                    resultDb = dbUncategorizedRaces.insert(dbPath);
                                    inserted = resultDb.second;
                                }*/
                            }
                    }
                }else{
                    LOGE("ABC-MISSING: either of asyncblocks %d or %d are missing entries in abcAsyncMap", op1->asyncId, op2->asyncId);
                }               
            }else{
                LOGE("ABC-MISSING: our technique is not supposed to mark R/W on non MQ thread as async race."
                    " Anamoly detected for accessSetIds %d and %d", o1, o2);
            } 
        }else{
            LOGE("ABC-MISSSING: either of operations %d and %d do not have an entry in abcTrace", o1, o2);
        }
    }
  
    //print only one race per category per field
    if(inserted){
        std::string accType1 = "";
        std::string accType2 = "";

        if(acc1->accessType == ABC_READ)
            accType1 = "READ";
        else
            accType1 = "WRITE";

        if(acc2->accessType == ABC_READ)
            accType2 = "READ";
        else
            accType2 = "WRITE";

        std::string clazz1, clazz2;
        if(acc1->clazz == NULL){
            clazz1 = "";
        }else{
            clazz1 = acc1->clazz;
        }
        if(acc2->clazz == NULL){
            clazz2 = "";
        }else{
            clazz2 = acc2->clazz;
        }

        std::ofstream outfile;
        outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
        outfile << "RACE rwId1:" << rwId1 << " type - " << accType1 << "  obj - "
                << acc1->obj << "  class - " << clazz1 << " field - " << acc1->field << "  fieldIdx - "
                <<  acc1->fieldIdx << "  dbPath - " << acc1->dbPath << "  tid - "
                << acc1->tid << "  accessId - " << acc1->accessId << "\n";
        outfile << "     rwId2:" << rwId2 << " type - " << accType2 << "  obj - "
                << acc2->obj << "  class - " << clazz1 << " field - " << acc2->field  << "  fieldIdx - "
                <<  acc2->fieldIdx << "  dbPath - " << acc2->dbPath << "  tid - "
                << acc2->tid << "  accessId - " << acc2->accessId << "\n\n";
        outfile.close();

    }
}

/*we dont report database races as we do not track them in detail*/
void printRacesDetectedToFile(){
    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << "\n\nCategorization of detected races\n" << "\n";
    outfile << "Multithreaded races" << "\n" ;

    std::set<std::pair<const char*, u4> >::iterator classIt;
    std::set<std::string>::iterator dbIt;
    std::map<std::pair<const char*, u4>, std::pair<int, int> >::iterator classMapIt;
    std::map<std::pair<Object*, u4>, std::pair<int, int> >::iterator objMapIt;
    std::map<std::string, std::pair<int, int> >::iterator dbMapIt;

    classIt = multiThreadRaces.begin();
    while(classIt != multiThreadRaces.end()){
        outfile << "class: " << (*classIt).first << "  field:" << (*classIt).second << "\n";
        ++classIt;
    }

/*    dbIt = dbMultiThreadRaces.begin();
    while(dbIt != dbMultiThreadRaces.end()){
        outfile << "database path: " << (*dbIt).c_str() << "\n" ;
        ++dbIt;
    }*/

    outfile << "\nAsync races (on single thread)" << "\n" ;
    outfile << "\nRaces due to a delayed post in ancestor asyncblocks" << "\n" ;

    classIt = delayPostRaces.begin();
    while(classIt != delayPostRaces.end()){
        outfile << "class: " << (*classIt).first << "  field:" << (*classIt).second << "\n";
        ++classIt;
    }

/*    dbIt = dbDelayPostRaces.begin();
    while(dbIt != dbDelayPostRaces.end()){
        outfile << "database path: " << (*dbIt).c_str() << "\n" ;
        ++dbIt;
    }*/

    outfile << "\nRaces due to cross thread post in ancestor asyncblocks" << "\n";
    classIt = crossPostRaces.begin();
    while(classIt != crossPostRaces.end()){
        outfile << "class: " << (*classIt).first << "  field:" << (*classIt).second << "\n";
        ++classIt;
    }

/*    dbIt = dbCrossPostRaces.begin();
    while(dbIt != dbCrossPostRaces.end()){
        outfile << "database path: " << (*dbIt).c_str() << "\n" ;
        ++dbIt;
    }*/

    outfile << "\nRaces due to co-enabled events" << "\n";
    classMapIt = coEnabledEventUiRacesClass.begin();
    while(classMapIt != coEnabledEventUiRacesClass.end()){
        outfile << "class: " << classMapIt->first.first << "  field:" << classMapIt->first.second 
            << " co-enabled trigger opIds:" << classMapIt->second.first << ", " << classMapIt->second.second << "\n";
        ++classMapIt;
    }
    objMapIt = coEnabledEventUiRacesObj.begin();
    while(objMapIt != coEnabledEventUiRacesObj.end()){
        outfile << "object: " << objMapIt->first.first << "  field:" << objMapIt->first.second
            << " co-enabled trigger opIds:" << objMapIt->second.first << ", " << objMapIt->second.second << "\n";
        ++objMapIt;
    }
    
    outfile << "\n";   

    classMapIt = coEnabledEventNonUiRacesClass.begin();
    while(classMapIt != coEnabledEventNonUiRacesClass.end()){
        outfile << "class: " << classMapIt->first.first << "  field:" << classMapIt->first.second
            << " co-enabled trigger opIds:" << classMapIt->second.first << ", " << classMapIt->second.second << "\n";
        ++classMapIt;
    }
    objMapIt = coEnabledEventNonUiRacesObj.begin();
    while(objMapIt != coEnabledEventNonUiRacesObj.end()){
        outfile << "object: " << objMapIt->first.first << "  field:" << objMapIt->first.second
            << " co-enabled trigger opIds:" << objMapIt->second.first << ", " << objMapIt->second.second << "\n";
        ++objMapIt;
    }

/*    dbMapIt = dbCoEnabledEventRaces.begin();
    while(dbMapIt != dbCoEnabledEventRaces.end()){
        outfile << "database path: " << dbMapIt->first 
            << " co-enabled trigger opIds:" << dbMapIt->second.first << ", " << dbMapIt->second.second << "\n";
        ++dbMapIt;
    }*/

    outfile << "\n Uncategorized races\n" ;
    classIt = uncategorizedRaces.begin();
    while(classIt != uncategorizedRaces.end()){
        outfile << "class: " << (*classIt).first << "  field:" << (*classIt).second << "\n";
        ++classIt;
    }

    dbIt = dbUncategorizedRaces.begin();
    while(dbIt != dbUncategorizedRaces.end()){
        outfile << "database path: " << (*dbIt).c_str() << "\n" ;
        ++dbIt;
    }
    
    outfile.close();

}


void detectRaceBetweenTwoSetOfOps(std::set<int> set1, std::set<int> set2, bool isWriteWrite){
    int i, j ;
    i = 0;
    j = 0;
    for(std::set<int>::iterator it1 = set1.begin(); it1 != set1.end(); ++it1){
        j = 0;
        for(std::set<int>::iterator it2 = set2.begin(); it2 != set2.end(); ++it2){
            //if(*it1 != *it2){
            if(!isWriteWrite || (isWriteWrite && (i < j))){
            int op1 = getOpIdOfReadWrite(*it1);
            int op2 = getOpIdOfReadWrite(*it2);
            if((op1 != -1 && op2 != -1) && op1 != op2 && 
                    !adjGraph[op1 - 1][op2 - 1] && !adjGraph[op2 - 1][op1 - 1]){
                    std::map<int, AbcRWAccess*>::iterator ii1 = abcRWAccesses.find(*it1);
                    std::map<int, AbcRWAccess*>::iterator ii2 = abcRWAccesses.find(*it2);                
                    if(ii1 != abcRWAccesses.end() && ii2 != abcRWAccesses.end()){

                    AbcRWAccess* acc1 = ii1->second;
                    AbcRWAccess* acc2 = ii2->second;
                          
                    if(acc1 != NULL && acc2 != NULL){
                    //needed only to categorize race based on some pre-defined natures
                    collectStatsOnTheRace(*it1, *it2, acc1, acc2, op1, op2);
                }
            }else{
                LOGE("ABC-MISSING: among rwids %d and %d one of the accesses is missing corresponding abcRWAccesses entry", *it1, *it2);
            } 

            }
            }
            //}
           
            ++j;
        }
        
        ++i;
    } 
}

void detectRaceUsingHbGraph(){
    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << "\n\nRaces detected\n" << "\n";
    outfile.close();

    //iterate over Object access map
    for(std::map<std::pair<Object*, u4>, std::pair<std::set<int>, std::set<int> > >::iterator it
            = abcObjectAccessMap.begin(); it != abcObjectAccessMap.end(); ++it){
        //detect read-write race
        detectRaceBetweenTwoSetOfOps(it->second.first, it->second.second, false);
        //detect write-write race
        detectRaceBetweenTwoSetOfOps(it->second.second, it->second.second, true);
    }

    //iterate over Database map
    for(std::map<std::string, std::pair<std::set<int>, std::set<int> > >::iterator it
            = abcDatabaseAccessMap.begin(); it != abcDatabaseAccessMap.end(); ++it){
        //detect read-write race
        detectRaceBetweenTwoSetOfOps(it->second.first, it->second.second, false);
        //detect write-write race
        detectRaceBetweenTwoSetOfOps(it->second.second, it->second.second, true);
    }

    //iterate over static access map
    for(std::map<std::pair<const char*, u4>, std::pair<std::set<int>, std::set<int> > >::iterator it
            = abcStaticAccessMap.begin(); it != abcStaticAccessMap.end(); ++it){
        //detect read-write race
        detectRaceBetweenTwoSetOfOps(it->second.first, it->second.second, false);
        //detect write-write race
        detectRaceBetweenTwoSetOfOps(it->second.second, it->second.second, true);
    }
}


bool abcPerformRaceDetection(){
    ThreadStatus oldStatus = dvmThreadSelf()->status;
    dvmThreadSelf()->status = THREAD_VMWAIT;
    //cleanup temporary maps
    if(abcThreadAccessSetMap.size() > 0){
        std::map<int, int>::iterator itTmp = abcThreadAccessSetMap.begin();
        while(itTmp != abcThreadAccessSetMap.end()){
        //    inserted = addIntermediateReadWritesToTrace(abcOpCount, itTmp->first);
            addAccessToTrace(abcOpCount++, itTmp->first, itTmp->second);
            abcThreadAccessSetMap.erase(itTmp++);
        }
    }
 
    if(abcThreadCurAsyncMap.size() > 0){
        std::map<int, AbcCurAsync*>::iterator itTmp = abcThreadCurAsyncMap.begin();
        while(itTmp != abcThreadCurAsyncMap.end()){
            AbcCurAsync* tmpPtr = itTmp->second;
            abcThreadCurAsyncMap.erase(itTmp++);
            free(tmpPtr);
        } 
    }

    if(abcAsyncStateMap.size() > 0){
        std::map<int, std::pair<bool,bool> >::iterator itTmp = abcAsyncStateMap.begin();
        while(itTmp != abcAsyncStateMap.end()){
            abcAsyncStateMap.erase(itTmp++);
        }
    }

    if(abcViewEventMap.size() > 0){
        std::map<u4, std::set<int> >::iterator itTmp = abcViewEventMap.begin();
        while(itTmp != abcViewEventMap.end()){
            abcViewEventMap.erase(itTmp++);
        }
    }

    if(abcLockCountMap.size() > 0){
        std::map<Object*, AbcLockCount*>::iterator itTmp = abcLockCountMap.begin();
        while(itTmp != abcLockCountMap.end()){
            abcLockCountMap.erase(itTmp++);
        }
    }

    if(abcDelayedReceiverTriggerThreadMap.size() > 0){
        std::map<int, AbcReceiver*>::iterator itTmp = abcDelayedReceiverTriggerThreadMap.begin();
        while(itTmp != abcDelayedReceiverTriggerThreadMap.end()){
            AbcReceiver* tmpPtr = itTmp->second;
            abcDelayedReceiverTriggerThreadMap.erase(itTmp++);
          //  free(tmpPtr->component);
            free(tmpPtr->action);
            free(tmpPtr);
        }
    }

    if(abcDelayedReceiverTriggerMsgMap.size() > 0){
        std::map<int, AbcReceiver*>::iterator itTmp = abcDelayedReceiverTriggerMsgMap.begin();
        while(itTmp != abcDelayedReceiverTriggerMsgMap.end()){
            AbcReceiver* tmpPtr = itTmp->second;
            abcDelayedReceiverTriggerMsgMap.erase(itTmp++);
         //   free(tmpPtr->component);
            free(tmpPtr->action);
            free(tmpPtr);
        }
    }

    //perform cleanup to reduce trace size 
    std::set<int> requiredEventEnableOps;
    std::map<int, AbcOp*>::iterator itTmp = abcTrace.begin();
    while(itTmp != abcTrace.end()){
        std::pair<u4, int> eventPair;
        std::map<std::pair<u4, int>, std::pair<int,int> >::iterator it;
        AbcOp* op = itTmp->second;
        switch(itTmp->second->opType){
        case ABC_ENABLE_EVENT:
             eventPair = std::make_pair(op->arg2->id, op->arg1);
             it = abcEnabledEventMap.find(eventPair);
             if(it == abcEnabledEventMap.end()){
                 abcEnabledEventMap.insert(std::make_pair(eventPair, std::make_pair(itTmp->first, -1)));
             }else{
                 it->second.first = itTmp->first;
             }
             break;
        case ABC_TRIGGER_EVENT:
             eventPair = std::make_pair(op->arg2->id, op->arg1);
             it = abcEnabledEventMap.find(eventPair);
             if(it != abcEnabledEventMap.end()){
                 requiredEventEnableOps.insert(it->second.first);
             }else{
                 LOGE("ABC: found a trigger event without corresponding enable. view:%d event %d", op->arg2->id, op->arg1);
                 gDvm.isRunABC = false;
                 dvmThreadSelf()->status = oldStatus;
                 return false;
             }
             break;
        } 
        ++itTmp;
    }

    //delete unused operations on trace and copy reduced trace to another map
    itTmp = abcTrace.begin();
    std::map<int, int> oldToNewDelayedReceiverTriggerMap;
    int traceIndex = abcStartOpId;
    while(itTmp != abcTrace.end()){
        AbcOp* op = itTmp->second;
        bool shouldDelete = false;
        if(op->opType == ABC_ENABLE_EVENT){
            if(requiredEventEnableOps.find(itTmp->first) == requiredEventEnableOps.end()){
                shouldDelete = true;
            }
        }else if(op->opType == ABC_POST){
            //delete posts for which corresponding calls are not seen
            if(op->arg3 == -1){
                shouldDelete = true;
            }
        }else if(op->opType == ABC_ACCESS){
              shouldDelete = false; //dummy statement
        }

        if(shouldDelete){
            abcTrace.erase(itTmp++);
            free(op);
        }else{
            if(traceIndex != itTmp->first){
                itTmp++;
            }else{
                ++itTmp;
            }
            ++traceIndex;
        }
    }

    for(std::map<std::pair<u4, int>, std::pair<int,int> >::iterator itr = abcEnabledEventMap.begin();
            itr != abcEnabledEventMap.end(); ){
        abcEnabledEventMap.erase(itr++);
    }

    LOGE("ABC: Trace generation and optimization completed. Trace processing begins.");

    //graph creation (performed after all optimizations performed on trace to reduce it
    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << " \n \n \n HB Graph \n \n";
    outfile.close();

    //do some initialization before processing starts
    adjGraph = (bool **)malloc(sizeof(bool*) * abcOpCount);
    for(int i = 0; i < abcOpCount; i++) {
        adjGraph[i] = (bool *)malloc(sizeof(bool) * abcOpCount);
    }

    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << " \n HB Graph size : " << abcTrace.size() << std::endl;
    outfile.close();

    //initially the graph has no edges
    for(int i=0; i< abcOpCount; i++){
        for(int j=0; j< abcOpCount; j++){
            adjGraph[i][j] = false;
        }
    }

    //initialize blankEnableResumeOp which tracks the most recent enable resume with blank instance
    blankEnableResumeOp = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
    blankEnableResumeOp->opId = -1;
    blankEnableResumeOp->opPtr = NULL;

    std::map<int, AbcOp*>::iterator opIt = abcTrace.begin();
    if(opIt->second->opType == ABC_START){
        Destination* dest1 = NULL;
        Source* src1 = NULL;
        adjMap.insert(std::make_pair(opIt->first, std::make_pair(dest1,src1)));

        ++opIt; 
        //2nd operation must be a threadinit for main thread
        if(opIt->second->opType == ABC_THREADINIT){
            Destination* dest2 = NULL;
            Source* src2 = NULL;
            adjMap.insert(std::make_pair(opIt->first, std::make_pair(dest2,src2)));

            AbcThreadBookKeep* newThread = (AbcThreadBookKeep*)malloc(sizeof(AbcThreadBookKeep));
            newThread->forkId = -1;
            newThread->loopId = -1;
            newThread->curAsyncId = -1;
            newThread->prevOpId = opIt->first;
            newThread->attachqId = -1;

            abcThreadBookKeepMap.insert(std::make_pair(opIt->second->arg1, newThread));
            addEdgeToHBGraph(abcStartOpId, opIt->first);
 
            ++opIt;
        }else{
            //cannot proceed further as trace is malformed
            LOGE("ABC: main thread THREADINIT missing in trace. Processing aborted.");
        }
    }else{
        //cannot proceed further as trace is malformed
        LOGE("ABC: START missing in trace. Processing aborted.");
    } 
    
    bool shouldAbort = false;
    for( ; opIt != abcTrace.end(); ++opIt){

        shouldAbort = false;
        int opId = opIt->first;
        AbcOp* op = opIt->second;
        LOGE("ABC: starting processing opid %d opType:%d", opId, op->opType);

        Destination* dest = NULL;
        Source* src = NULL;
        adjMap.insert(std::make_pair(opId, std::make_pair(dest,src)));

        if(op->opType == ABC_NATIVE_ENTRY){
            shouldAbort = processNativeEntryOperation(opId, op);
            if(shouldAbort){
                dvmThreadSelf()->status = oldStatus;
                return false;
            }
        }else{

        std::map<int, AbcThreadBookKeep*>::iterator threadIt = abcThreadBookKeepMap.find(op->tid);
        if(threadIt != abcThreadBookKeepMap.end()){
         
        switch(op->opType){
        case ABC_THREADINIT :
             processThreadinitOperation(opId, op, threadIt->second);
             break;
        case ABC_THREADEXIT :
             checkAndAdd_PO_edge(opId, op, threadIt->second->prevOpId);
             processThreadexitOperation(opId, op);
             break;
        case ABC_FORK :
             shouldAbort = processForkOperation(opId, op, threadIt->second);
             break;
        case ABC_WAIT:
             processWaitOperation(opId, op, threadIt->second);
             break;
        case ABC_NOTIFY:
             shouldAbort = processNotifyOperation(opId, op, threadIt->second);
             break;
        case ABC_JOIN : //taken care by notify-wait semantics
             break;
        case ABC_ATTACH_Q :
             processAttachqOperation(opId, op, threadIt->second);
             break;
        case ABC_LOOP :
             processLoopOperation(opId, op, threadIt->second);
             break;
        case ABC_ADD_IDLE_HANDLER:
             processAddIdleHandlerOperation(opId, op, threadIt->second);
             break;
        case ABC_QUEUE_IDLE:
             processQueueIdleOperation(opId, op, threadIt->second);
             break;
        case ABC_REMOVE_IDLE_HANDLER:
             processRemoveIdleHandlerOperation(opId, op, threadIt->second);
             break;
        case ABC_POST :
             shouldAbort = processPostOperation(opId, op, threadIt->second);
             break;
        case ABC_CALL :
             shouldAbort = processCallOperation(opId, op, threadIt->second);
             break;
        case ABC_RET :
             processRetOperation(opId, op, threadIt->second);
             break;
        case ABC_LOCK :
             processLockOperation(opId, op, threadIt->second);
             break;
        case ABC_UNLOCK :
             shouldAbort = processUnlockOperation(opId, op, threadIt->second);
             break;
        case ABC_ACCESS :
             processAccessOperation(opId, op, threadIt->second);
             break;
        case ABC_NATIVE_EXIT :
             processNativeExitOperation(opId, op);
             break;
        case ABC_ENABLE_EVENT :
             processEnableEventOperation(opId, op, threadIt->second); 
             break;
        case ABC_TRIGGER_EVENT :
             shouldAbort = processTriggerEventOperation(opId, op, threadIt->second);       
             break;
        case ENABLE_WINDOW_FOCUS :
             processEnableWindowFocusChange(opId, op, threadIt->second);
             break;
        case TRIGGER_WINDOW_FOCUS :
             processTriggerWindowFocusChange(opId, op, threadIt->second);
             break;
        case ABC_ENABLE_LIFECYCLE:
             processEnableLifecycleOperation(opId, op, threadIt->second);
             break;
        case ABC_TRIGGER_LIFECYCLE:
             shouldAbort = processTriggerLifecycleOperation(opId, op, threadIt->second);
             break;
        case ABC_INSTANCE_INTENT:
             shouldAbort = processInstanceIntentMap(opId, op, threadIt->second);
             break;
        case ABC_TRIGGER_SERVICE:
             processTriggerServiceLifecycleOperation(opId, op, threadIt->second);
             break;
        case ABC_TRIGGER_RECEIVER:
             shouldAbort = processTriggerBroadcastReceiver(opId, op, threadIt->second);
             break;
        default: LOGE("ABC: found an unknown opType when processing abcTrace. Aborting.");
                 gDvm.isRunABC = false;
             dvmThreadSelf()->status = oldStatus;
             return false;
        } 
        
        if(op->opType != ABC_THREADEXIT && op->opType != ABC_INSTANCE_INTENT && 
                    op->opType != ABC_REMOVE_IDLE_HANDLER){
            if(shouldAbort || checkAndAbortIfAssumtionsFailed(opId, op, threadIt->second)){
              //  break;
                dvmThreadSelf()->status = oldStatus;
                return false;
            }
            checkAndAdd_PO_edge(opId, op, threadIt->second->prevOpId);
            if(op->opType != ABC_NATIVE_EXIT && op->opType != ABC_RET && op->opType != ABC_LOOP){
                threadIt->second->prevOpId = opId;
            }else{
                threadIt->second->prevOpId = -1;
            }
        }

        }else{
            LOGE("ABC: an operation associated with no thread or logged before its"
                " corresponding thread creation log found. Trace processing aborted.");
            dvmThreadSelf()->status = oldStatus;
            return false;
        }
        }
    }

    LOGE("HB base graph creation completed");
    //closure algorithm
    computeClosureOfHbGraph();
    LOGE("HB graph creation completed");

    //race detection
    removeRedundantReadWrites();
    detectRaceUsingHbGraph();
    LOGE("ABC: Race detection completed");
    
    dvmThreadSelf()->status = oldStatus;

    return true;
}


void abcComputeMemoryUsedByRaceDetector(){
    std::ofstream outfile;
    outfile.open(gDvm.abcLogFile.c_str(), std::ios_base::app);
    outfile << "\n\nMemory used for race detection\n" << "\n";
    outfile << "AbcTrace         : " << abcTrace.size() << " * " << sizeof(AbcOp) << "\n"; 
    outfile << "AbcAsyncMap      : " << abcAsyncMap.size() << " * " << sizeof(AbcAsync) << "\n";
    outfile << "ReadWriteMap     : " << abcRWAccesses.size() << " * " << sizeof(AbcRWAccess) << "\n";
    outfile << "ThreadBookKeepMap: " << abcThreadBookKeepMap.size() << " * " << sizeof(AbcThreadBookKeep) << "\n";
    outfile << "LockMap          : " << abcLockMap.size() << " * " << sizeof(AbcLock) << "\n";   
    outfile << "Graph size       : " << abcTrace.size() << " * " << abcTrace.size() << " * " << sizeof(bool) << "\n";


    outfile.close();
}

void abcPrintRacesDetectedToFile(){
    printRacesDetectedToFile();
}

int abcGetTraceLength(){
    int traceLength = abcOpCount + (abcRWCount - abcAccessSetCount);
    return traceLength;
}

int abcGetThreadCount(){
    return abcForkCount;
}

int abcGetMessageQueueCount(){
    return abcMQCount;
}

int abcGetAsyncBlockCount(){
    int asyncCount = abcAsyncMap.size();
    return asyncCount;
}

int abcGetEventTriggerCount(){
    return abcEventTriggerCount;
}

//this is a sum of (class + field) + database
int abcGetFieldCount(){
    int fieldCount = fieldSet.size();
    return fieldCount;
}

int abcGetMultiThreadedRaceCount(){
    int count = multiThreadRacesObj.size() + multiThreadRacesClass.size();
    return count;
}

//this is some dummy thing..does not matter what you compute here 
int abcGetAsyncRaceCount(){
    int count = raceyFieldSet.size() + dbRaceyFieldSet.size() ;
    return count;
}

int abcGetDelayPostRaceCount(){
    int count = delayPostRacesObj.size() + delayPostRacesClass.size();
    return count;
}

int abcGetCrossPostRaceCount(){
    int count = crossPostRacesObj.size() + crossPostRacesClass.size();
    return count;
}

int abcGetCoEnabledEventUiRaces(){
    int count = coEnabledEventUiRacesObj.size() + coEnabledEventUiRacesClass.size();
    return count;
}

int abcGetCoEnabledEventNonUiRaces(){
    int count = coEnabledEventNonUiRacesObj.size() + coEnabledEventNonUiRacesClass.size();
    return count;
}
