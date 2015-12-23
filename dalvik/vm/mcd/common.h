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

/* This is a common header file for Android bug-checker native code
 *
 * @author Pallavi Maiya
 */

/*todo:
 *(1)if this class works fine, after initalizing along with gDvm,
 *then consider storing custom fields currently in gDvm here iself
 *This will pack all ABC related things in one place unless
 *unavoidable.
*/

/* todo: have a tag to identify threads started by Thread.start() and when 
 * shouldAbcTrack set to true and the rest (threads started by native method
 * or library and later hitting app code). 
 */

#ifndef COMMON_H_
#define COMMON_H_

#include "Dalvik.h"
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <limits.h>

struct ArgumentStruct{
    Object* obj; //lock
    u4 id; //thread-id/msg-id/access-id
};
typedef struct ArgumentStruct AbcArg;

struct operationStruct{
    int opType;
    int arg1;
    AbcArg* arg2;
    int arg3;
    int arg4; //delay argument for post (-1 indicates front of queue)
   // int followingOpId;
    char* arg5; //holds actionof broadcast receiver 
    int tid; //thread executing the operation
    int asyncId; //async-block from ehich the operation is posted; -1 if posted from non MQ thread
    bool tbd; //true if node to be deleted
};
typedef struct operationStruct AbcOp;

struct AbcOperation{
    int opId;
    AbcOp* opPtr;
};
typedef struct AbcOperation AbcOpWithId;

struct asyncStruct{
  //  int msg;
    int tid;
    int postId;
    int callId;
    int retId;
    int parentAsyncId; //-1 if posted by a non-queue thread
    int delay; //stored here only to reduce lookup
   
    //fields needed to collect stats for nature of async races
    int recentTriggerOpId; //tracks the most recent trigger that led to this async block being called
    int recentCrossPostAsync; //tracks the most recent ancestor that was posted from another thread
    int recentDelayAsync; //tracks most recent ancestor that was posted with a delay
};
typedef struct asyncStruct AbcAsync;

struct WorklistElemStruct{
    int src;
    int dest;
    struct WorklistElemStruct * prev;
};
typedef struct WorklistElemStruct WorklistElem;

struct DestinationStruct{
    int dest;
    struct DestinationStruct * prev;
};
typedef struct DestinationStruct Destination;

struct SourceStruct{
    int src;
    struct SourceStruct * prev;
};
typedef struct SourceStruct Source;


extern int abcOpCount;
/*program trace stored as hashmap with key being the index 
 *of operation in the trace*/
extern std::map<int, AbcOp*> abcTrace;
extern bool ** adjGraph;
extern WorklistElem* worklist;
extern std::map<int, std::pair<Destination*, Source*> > adjMap;
extern std::map<int, AbcAsync*> abcAsyncMap;


bool isHbEdge(int src, int dest);
void addEdgeToHBGraph(int op1, int op2);
AbcAsync* getAsyncBlockFromId(int asyncId);
int getAsyncIdOfOperation(int opId);

#endif  //common.h
