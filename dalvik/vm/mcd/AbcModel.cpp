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
 * concurrency bugs. This file has functions to model Android app 
 * components
 *
 * @author Pallavi Maiya
 */


#include "AbcModel.h"

/*Activity lifecycle related datastructures*/

//a mapping from activity instance to intentID
std::map<u4, int> AbcInstanceIntentMap;
//a mapping from activity instance (or intentID temporarily) 
//to a TRIGGER op
std::map<u4, AbcOpWithId*> ActivityStateMap;
//activity instance is the key
std::map<u4, AbcActivityResult*> ActivityResultMap;
std::map<std::pair<u4, int>, AbcEnableTriggerList*> AbcEnableTriggerLcMap;

AbcOpWithId* blankEnableResumeOp = NULL;

/*Service lifecycle related datastructures*/
std::map<std::string, AbcService*> AbcServiceMap;
std::map<u4, AbcOpWithId*> AbcRequestStartServiceMap;
std::map<u4, AbcRequestBind*> AbcServiceConnectMap;

/*Broadcast receiver related datastructures*/
std::set<std::string> StickyIntentActionSet;
//key: <componentHash, action>  value:first registerReceiver request for the comp-action pair
//entry removed on seeing unregister
std::map<std::pair<u4, std::string>, int> RegisterReceiverMap;
//key: <componentHash, action>  value: list of registerRec opId when corresponding
//action had a sticky intent
std::map<std::pair<u4, std::string>, std::list<int> > StickyRegisterReceiverMap;
//<intentId - intentInfo>
std::map<int, AbcSticky*> SentIntentMap;
std::map<int, OnReceiveLater*> PreReceiverTriggerToRegisterMap;
std::map<int, AbcSticky*> AbcRegisterOnReceiveMap;
std::map<int, std::list<AbcOpWithId*> > AbcSendBroadcastOnReceiveMap;

int timeTickBroadcastPrevPostOpId = -1;
int abcAppBindPost = -1;


void addTriggerToTriggerBaseEdges(AbcOp* curOp, AbcOp* prevOp, int curOpid, 
        int prevOpid, AbcAsync* curOpAsync, AbcAsync* prevOpAsync){
    addEdgeToHBGraph(prevOpid, curOpid);
    if(prevOp->asyncId != curOp->asyncId &&
        prevOp->tid == curOp->tid){
        addEdgeToHBGraph(prevOpAsync->retId, curOpAsync->callId);
        addEdgeToHBGraph(prevOpAsync->postId, curOpAsync->postId);
    }
}

bool checkAndUpdateBroadcastState(int opId, AbcOp* op){
    bool updated = false;
    int state = op->arg1;
    u4 component = op->arg2->id;
    int intentId = op->arg3;
    std::string action(op->arg5);
    
    if(state == ABC_SEND_BROADCAST){
        AbcSticky* as = (AbcSticky*)malloc(sizeof(AbcSticky));
        as->op = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
        as->op->opId = opId;
        as->op->opPtr = op;
        as->isSticky = false;
        SentIntentMap.insert(std::make_pair(intentId, as));
       
        updated = true;
    }else if(state == ABC_SEND_STICKY_BROADCAST){
        AbcSticky* as = (AbcSticky*)malloc(sizeof(AbcSticky));
        as->op = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
        as->op->opId = opId;
        as->op->opPtr = op;
        as->isSticky = true;
        SentIntentMap.insert(std::make_pair(intentId, as));

        StickyIntentActionSet.insert(action);
        updated = true;
    }else if(state == ABC_REGISTER_RECEIVER){
        std::pair<u4, std::string> compActionPair = std::make_pair(component, action);
        RegisterReceiverMap.insert(std::make_pair(compActionPair, opId));
        
        //check if corresponding action has sticky intent
        if(StickyIntentActionSet.find(action) != StickyIntentActionSet.end()){
            std::map<std::pair<u4, std::string>, std::list<int> >::iterator it =
                     StickyRegisterReceiverMap.find(compActionPair);
            if(it != StickyRegisterReceiverMap.end()){
                it->second.push_back(opId);
            }else{
                std::list<int> registerList;
                registerList.push_back(opId);
                StickyRegisterReceiverMap.insert(std::make_pair(compActionPair, registerList));
            }
        }

        updated = true;
    }else if(state == ABC_UNREGISTER_RECEIVER){
        std::map<std::pair<u4, std::string>, int>::iterator regIt = RegisterReceiverMap.begin();
        for( ; regIt != RegisterReceiverMap.end(); ){
            if(regIt->first.first == component){
                RegisterReceiverMap.erase(regIt++);
            }else{
                ++regIt;
            }
        }

        //maybe this cleanup is not needed as each registerReceiver called when a 
        //a corresponding sticky intent is around is supposed to thrigger onReceive
        //which in turn will clear the corresponding entry in this map. 
        std::map<std::pair<u4, std::string>, std::list<int> >::iterator stickyIt = 
                StickyRegisterReceiverMap.begin();
        for( ; stickyIt != StickyRegisterReceiverMap.end(); ){
            if(stickyIt->first.first == component){
                stickyIt->second.clear();
                StickyRegisterReceiverMap.erase(stickyIt++);
            }else{
                ++stickyIt;
            }
        }

        updated = true;
    }else if(state == ABC_REMOVE_STICKY_BROADCAST){
        StickyIntentActionSet.erase(action);
        updated = true;
    }else if(state == ABC_TRIGGER_ONRECIEVE_LATER){
        std::pair<u4, std::string> compActionPair = std::make_pair(component, action);

        std::map<int, AbcSticky*>::iterator siIt = SentIntentMap.find(intentId);
        if(siIt != SentIntentMap.end()){
            if(siIt->second->isSticky){
                std::map<std::pair<u4, std::string>, std::list<int> >::iterator stickyIt =
                    StickyRegisterReceiverMap.find(compActionPair);
                if(stickyIt != StickyRegisterReceiverMap.end() && !stickyIt->second.empty()){
                //if onReceive is due to registerBroadcastReceiver for an existing sticky intent
                    OnReceiveLater* orl = (OnReceiveLater*)malloc(sizeof(OnReceiveLater));
                    orl->sendIntentOpid = siIt->second->op->opId;
                    orl->registerReceiverOpid = stickyIt->second.front();
                    orl->isSticky = true;
                    PreReceiverTriggerToRegisterMap.insert(std::make_pair(opId, orl));
                    stickyIt->second.pop_front();
                }else{
                //if onReceive is due to sendStickyIntent for an already registered reciever
                    std::map<std::pair<u4, std::string>, int>::iterator recIter =
                        RegisterReceiverMap.find(compActionPair);
                    if(recIter != RegisterReceiverMap.end()){
                        OnReceiveLater* orl = (OnReceiveLater*)malloc(sizeof(OnReceiveLater));
                        orl->sendIntentOpid = siIt->second->op->opId;
                        orl->registerReceiverOpid = recIter->second;
                        //sticky-register rule cant be applied in this case as register
                        //happened first and sendStickyIntent next
                        orl->isSticky = false; 

                        PreReceiverTriggerToRegisterMap.insert(std::make_pair(opId, orl));
                    }else{
                        //probably this is a manifest registered receiver & hence has no corresponding REGISTER cpmmand
                        OnReceiveLater* orl = (OnReceiveLater*)malloc(sizeof(OnReceiveLater));
                        orl->sendIntentOpid = siIt->second->op->opId;
                        orl->registerReceiverOpid = -1;
                        //sticky-register rule cant be applied in this case as register
                        //happened first and sendStickyIntent next
                        orl->isSticky = false; 

                        PreReceiverTriggerToRegisterMap.insert(std::make_pair(opId, orl));
                    }
                }        
            }else{
                //corresponding intent is not sticky; only check for non-sticky REGISTER broadcasts
                std::map<std::pair<u4, std::string>, int>::iterator recIter =
                    RegisterReceiverMap.find(compActionPair);
                if(recIter != RegisterReceiverMap.end()){
                    OnReceiveLater* orl = (OnReceiveLater*)malloc(sizeof(OnReceiveLater));
                    orl->sendIntentOpid = siIt->second->op->opId;
                    orl->registerReceiverOpid = recIter->second;
                    //sticky-register rule cant be applied in this case as register
                    //happened first and sendStickyIntent next
                    orl->isSticky = false; 
                    PreReceiverTriggerToRegisterMap.insert(std::make_pair(opId, orl));
                }else{
                    //probably this is a manifest registered receiver & hence has no corresponding REGISTER cpmmand
                    OnReceiveLater* orl = (OnReceiveLater*)malloc(sizeof(OnReceiveLater));
                    orl->sendIntentOpid = siIt->second->op->opId;
                    orl->registerReceiverOpid = -1;
                    orl->isSticky = false;
                    PreReceiverTriggerToRegisterMap.insert(std::make_pair(opId, orl));
                }
            }
        }else{
            //no corresponding sendIntent (intent sent from system process or other apps and not tracked)
            //corresponding intent is not sticky; only check for non-sticky REGISTER broadcasts
            std::map<std::pair<u4, std::string>, int>::iterator recIter =
                RegisterReceiverMap.find(compActionPair);
            if(recIter != RegisterReceiverMap.end()){
                OnReceiveLater* orl = (OnReceiveLater*)malloc(sizeof(OnReceiveLater));
                orl->sendIntentOpid = -1;
                orl->registerReceiverOpid = recIter->second;
                orl->isSticky = false;
                PreReceiverTriggerToRegisterMap.insert(std::make_pair(opId, orl));
            }else{
                //probably this is a manifest registered receiver & hence has no corresponding REGISTER cpmmand
                OnReceiveLater* orl = (OnReceiveLater*)malloc(sizeof(OnReceiveLater));
                orl->sendIntentOpid = -1;
                orl->registerReceiverOpid = -1;
                orl->isSticky = false;
                PreReceiverTriggerToRegisterMap.insert(std::make_pair(opId, orl));
            }            
        }
        
        updated = true;
    }else if(state == ABC_TRIGGER_ONRECIEVE){
        AbcAsync* opAsync = getAsyncBlockFromId(op->asyncId);
        if(opAsync == NULL){
            LOGE("ABC-ABORT: missing async block for onReceive of broadcast action: %s", action.c_str());
            gDvm.isRunABC = false;
            return updated;
        }

        //check if action is android.intent.action.TIME_TICK
        if(action == "android.intent.action.TIME_TICK"){
            //TIME_TICK is a periodic broadcast and hence inherently happens-before
            //exists between adjacent TIME_TICKs and thus between posts of corresponding onReceive
            if(timeTickBroadcastPrevPostOpId != -1){
                addEdgeToHBGraph(timeTickBroadcastPrevPostOpId, opAsync->postId);
            }
            timeTickBroadcastPrevPostOpId = opAsync->postId;
        }

        //post for broadcast receiver could happen only after BIND_APPLICATION's post
        addEdgeToHBGraph(abcAppBindPost, opAsync->postId);

        std::pair<u4, std::string> compActionPair = std::make_pair(component, action);

        std::map<int, OnReceiveLater* >::iterator preIter = 
                PreReceiverTriggerToRegisterMap.find(op->arg4);
        if(preIter != PreReceiverTriggerToRegisterMap.end()){
            if(preIter->second->sendIntentOpid != -1){
                addEdgeToHBGraph(preIter->second->sendIntentOpid, opId);
                addEdgeToHBGraph(preIter->second->sendIntentOpid, opAsync->postId);

                /*add entry to AbcSendBroadcastOnReceiveMap. This map is used to
                 *add the edge onReceive1 < onReceive2 if sendBroadcast1 < sendBroadcast2
                 *For this we should only consider those onReceives which are due to
                 *sendBroadcast and not due to registerReceiver for a sticky broadcast.
                 *Also, if the same broadcast had multiple receivers we log all
                 *of them
                 */
                if(!preIter->second->isSticky){
                    AbcOpWithId* astTmp = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
                    astTmp->opId = opId;
                    astTmp->opPtr = op;
                    
                    std::map<int, std::list<AbcOpWithId*> >::iterator brRecIt = 
                        AbcSendBroadcastOnReceiveMap.find(preIter->second->sendIntentOpid);
                    if(brRecIt == AbcSendBroadcastOnReceiveMap.end()){
                        std::list<AbcOpWithId*> onRecList;
                        onRecList.push_back(astTmp);
                        AbcSendBroadcastOnReceiveMap.insert(std::make_pair(
                            preIter->second->sendIntentOpid, onRecList));
                    }else{
                        brRecIt->second.push_back(astTmp);
                    }
                }
            }

            if(preIter->second->registerReceiverOpid != -1){
                addEdgeToHBGraph(preIter->second->registerReceiverOpid, opId);
                addEdgeToHBGraph(preIter->second->registerReceiverOpid, opAsync->postId);

                AbcSticky* ast = (AbcSticky*)malloc(sizeof(AbcSticky));            
                ast->op = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
                ast->op->opId = opId;
                ast->op->opPtr = op;
                ast->isSticky = preIter->second->isSticky;
                AbcRegisterOnReceiveMap.insert(std::make_pair(preIter->second->registerReceiverOpid, ast));
            }
        }else{
            int regOpid = -1; 
            bool isSticky = false;

            std::map<int, AbcSticky*>::iterator siIt = SentIntentMap.find(intentId);
            if(siIt != SentIntentMap.end()){
                //add edge from sendIntent to onReceive
                addEdgeToHBGraph(siIt->second->op->opId, opId);
                addEdgeToHBGraph(siIt->second->op->opId, opAsync->postId);

                //check if edge should be added from sticky register or otherwise
                if(siIt->second->isSticky){ 
                    std::map<std::pair<u4, std::string>, std::list<int> >::iterator stickyIt =
                        StickyRegisterReceiverMap.find(compActionPair);
                    if(stickyIt != StickyRegisterReceiverMap.end() && !stickyIt->second.empty()){
                        regOpid = stickyIt->second.front();
                        stickyIt->second.pop_front();
                        isSticky = true;
                    }else{
                        std::map<std::pair<u4, std::string>, int>::iterator recIter =
                            RegisterReceiverMap.find(compActionPair);
                        if(recIter != RegisterReceiverMap.end()){
                            regOpid = recIter->second;
                            isSticky = false;
                        }
                    }
                }else{
                    //onReceive corresponding to non-sticky intent
                    std::map<std::pair<u4, std::string>, int>::iterator recIter =
                            RegisterReceiverMap.find(compActionPair);
                    if(recIter != RegisterReceiverMap.end()){
                        regOpid = recIter->second;
                        isSticky = false;
                    }
                }

                /*add entry to AbcSendBroadcastOnReceiveMap. This map is used to
                 *add the edge onReceive1 < onReceive2 if sendBroadcast1 < sendBroadcast2
                 *For this we should only consider those onReceives which are due to
                 *sendBroadcast and not due to registerReceiver for a sticky broadcast
                 */
                if(!isSticky){
                    AbcOpWithId* astTmp = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
                    astTmp->opId = opId;
                    astTmp->opPtr = op;

                    std::map<int, std::list<AbcOpWithId*> >::iterator brRecIt =
                        AbcSendBroadcastOnReceiveMap.find(siIt->second->op->opId);
                    if(brRecIt == AbcSendBroadcastOnReceiveMap.end()){
                        std::list<AbcOpWithId*> onRecList;
                        onRecList.push_back(astTmp);
                        AbcSendBroadcastOnReceiveMap.insert(std::make_pair(
                            siIt->second->op->opId, onRecList));
                    }else{
                        brRecIt->second.push_back(astTmp);
                    }
                }
            }else{
                //no corresponding sendIntent for onReceive (i.e intent has been fired external to app)
                std::map<std::pair<u4, std::string>, int>::iterator recIter =
                        RegisterReceiverMap.find(compActionPair);
                if(recIter != RegisterReceiverMap.end()){
                    regOpid = recIter->second;
                    isSticky = false;
                }
            }

            //add edges for registerReceiver operation
            if(regOpid != -1){
                addEdgeToHBGraph(regOpid, opId);
                addEdgeToHBGraph(regOpid, opAsync->postId);

                AbcSticky* ast = (AbcSticky*)malloc(sizeof(AbcSticky));
                ast->op = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
                ast->op->opId = opId;
                ast->op->opPtr = op;
                ast->isSticky = isSticky;
                AbcRegisterOnReceiveMap.insert(std::make_pair(regOpid, ast));
            }

        }
        
        updated = true;
    }

    return updated;
}

bool checkAndUpdateServiceState(int opId, AbcOp* op){
    bool updated = false;
    int state = op->arg1;
    int identifier = op->arg2->id;

    std::string serviceName; //service class
    if(state != ABC_REQUEST_UNBIND_SERVICE){
        serviceName.assign(op->arg5);
        std::map<std::string, AbcService*>::iterator serIter = AbcServiceMap.find(serviceName); 
        std::map<u4, AbcRequestBind*>::iterator bindIter;
        
        if(state == ABC_REQUEST_START_SERVICE){
            AbcOpWithId* owi = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
            owi->opId = opId;
            owi->opPtr = op;
            AbcRequestStartServiceMap.insert(std::make_pair(identifier, owi));
            
            //check if this is the first request to start service
            if(serIter == AbcServiceMap.end()){
                AbcService* service = new AbcService;
                service->firstCreateServiceRequest = opId;

                service->prevStartedServiceOp = NULL;
                service->prevBoundServiceOp = NULL;
                service->firstStopServiceRequest = -1;
                service->firstBindServiceRequest = -1;
                service->createServiceOp = -1;
                service->bindServiceOp = -1;
                AbcServiceMap.insert(std::make_pair(serviceName, service));
            }

            updated = true;
            
        }
        else if(state == ABC_REQUEST_BIND_SERVICE){
            bindIter = AbcServiceConnectMap.find(identifier);
            if(bindIter == AbcServiceConnectMap.end()){
                AbcRequestBind* rb = new AbcRequestBind;
                rb->requestBindOp = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
                rb->requestBindOp->opId = opId;
                rb->requestBindOp->opPtr = op;
                
                rb->serviceClassname.assign(serviceName);
                rb->requestUnbindOp = -1;
                rb->prev = NULL;

                AbcServiceConnectMap.insert(std::make_pair(identifier, rb));

            }//end bindIter = NULL
            else{
                //search for entry corresponding to the service
                bool foundEntry = false;
                AbcRequestBind* tmpArb = bindIter->second;
                do{
                    if(tmpArb->serviceClassname == serviceName){
                        if(tmpArb->requestUnbindOp != -1){
                            //an unbind has been performed on this connection and hence can be rebound
                            //this will lose previous request info but this is best we can do as of now
                            //and may work for most of the cases
                            tmpArb->requestBindOp->opPtr = op;
                            tmpArb->requestBindOp->opId = opId;
                            tmpArb->requestUnbindOp = -1;
                        }
                        foundEntry = true;
                        break;
                    }
                    tmpArb = tmpArb->prev;
                }while(tmpArb != NULL);
                //this connection has been unbound. So rewrite its entries
                if(!foundEntry){
                    AbcRequestBind* rb = new AbcRequestBind;
                    rb->requestBindOp = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
                    rb->requestBindOp->opId = opId;
                    rb->requestBindOp->opPtr = op;

                    rb->serviceClassname.assign(serviceName);
                    rb->requestUnbindOp = -1;
                    rb->prev = bindIter->second;
                    bindIter->second = rb;
                }
            }

            //check if this is the first request to start service
            if(serIter == AbcServiceMap.end()){
                AbcService* service = new AbcService;
                service->firstCreateServiceRequest = opId;

                service->firstBindServiceRequest = opId;

                service->prevStartedServiceOp = NULL;
                service->prevBoundServiceOp = NULL;
                service->firstStopServiceRequest = -1;
                service->createServiceOp = -1;
                service->bindServiceOp = -1;

                service->connectionSet.insert(identifier);
                AbcServiceMap.insert(std::make_pair(serviceName, service));
            }else{
                if(serIter->second->firstBindServiceRequest == -1){
                    serIter->second->firstBindServiceRequest = opId;
                }
                serIter->second->connectionSet.insert(identifier);
            }

          updated = true;
        }
        else if(state == ABC_CREATE_SERVICE){
            if(serIter != AbcServiceMap.end() && serIter->second->createServiceOp == -1){
                AbcService* service = serIter->second;
                addEdgeToHBGraph(service->firstCreateServiceRequest, opId);
                AbcAsync* opAsync = getAsyncBlockFromId(op->asyncId);
                if(opAsync != NULL){
                    addEdgeToHBGraph(service->firstCreateServiceRequest, opAsync->postId);
                }else{
                    LOGE("ABC-ABORT: missing async block for CREATE-SERVICE of service: %s", serviceName.c_str());
                    gDvm.isRunABC = false;
                    return updated;
                }

                //add info to service datastructure
                service->createServiceOp = opId;
                
                service->prevStartedServiceOp = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
                service->prevStartedServiceOp->opId = opId;
                service->prevStartedServiceOp->opPtr = op;

                service->prevBoundServiceOp = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
                service->prevBoundServiceOp->opId = opId;
                service->prevBoundServiceOp->opPtr = op;

                updated = true;
            }else{
                LOGE("ABC-ABORT:CREATE-SERVICE hit without an entry in ServiceMap due to"
                     " bindService or startService; or hit with an existing CREATE-SERVICE." 
                     " Hit a service usage pattern that is not"
                     " supported by ABC for service %s", serviceName.c_str());
                gDvm.isRunABC = false;
                return updated;
            }

        }
        else if(state == ABC_BIND_SERVICE){
            if(serIter != AbcServiceMap.end() && serIter->second->createServiceOp != -1
                   && serIter->second->bindServiceOp == -1){
                AbcService* service = serIter->second;
                addEdgeToHBGraph(service->firstBindServiceRequest, opId);
                AbcAsync* opAsync = getAsyncBlockFromId(op->asyncId);
                if(opAsync != NULL){
                    addEdgeToHBGraph(service->firstBindServiceRequest, opAsync->postId);
                }else{
                    LOGE("ABC-ABORT: missing async block for BIND-SERVICE of service: %s", serviceName.c_str());
                    gDvm.isRunABC = false;
                    return updated;
                }

                //fill information in datastructure
                service->bindServiceOp = opId;

                //add edge from previous state to current one
                AbcOpWithId* prevOp = service->prevBoundServiceOp;
                AbcAsync* prevAsync;
                switch(prevOp->opPtr->arg1){
                  case ABC_UNBIND_SERVICE:
                  case ABC_CREATE_SERVICE:
                    prevAsync = getAsyncBlockFromId(prevOp->opPtr->asyncId);
                    addTriggerToTriggerBaseEdges(op, prevOp->opPtr, opId,
                        prevOp->opId, opAsync, prevAsync);
                    updated = true;
                  break;
                  default: LOGE("state %d of service %s encountered spurious previous state %d", state, serviceName.c_str(), prevOp->opPtr->arg1);
                     gDvm.isRunABC = false;
                     return updated;
                }

                if(updated){
                    service->prevBoundServiceOp->opId = opId;
                    service->prevBoundServiceOp->opPtr = op;            
                }
            }else{
                LOGE("ABC-ABORT: BIND-SERVICE seen without a prior CREATE-SERVICE or a ServiceMap entry"
                     " or with an existing BIND-SERVICE call, for service %s", serviceName.c_str());
                gDvm.isRunABC = false;
                return updated;
            }

        }
        else if(state == ABC_UNBIND_SERVICE){
            if(serIter != AbcServiceMap.end() && serIter->second->bindServiceOp != -1){
                AbcService* service = serIter->second;
                AbcAsync* opAsync = getAsyncBlockFromId(op->asyncId);
                if(opAsync == NULL){
                    LOGE("ABC-ABORT: missing async block for BIND-SERVICE of service: %s", serviceName.c_str());
                    gDvm.isRunABC = false;
                    return updated;
                }
                /*add edges from all previous valid (those made when an active connection was present)
                 *unbind requests to UNBIND-SERVICE as they are the cause for the service to exit its
                 *bounded state
                 */
                std::set<int>::iterator setIt = service->validRequestUnbindSet.begin();
                if(service->validRequestUnbindSet.size() > 0){
                    for( ; setIt != service->validRequestUnbindSet.end(); ++setIt){
                        addEdgeToHBGraph(*setIt, opId);
                        addEdgeToHBGraph(*setIt, opAsync->postId);
                    }
                }else{
                    LOGE("ABC-MISSING: UNBIND-SER made on service %s without any prior request unbind", serviceName.c_str());
                }

                //add edge from previous state to current one
                AbcOpWithId* prevOp = service->prevBoundServiceOp;
                AbcAsync* prevAsync;
                switch(prevOp->opPtr->arg1){
                  case ABC_BIND_SERVICE:
                    prevAsync = getAsyncBlockFromId(prevOp->opPtr->asyncId);
                    addTriggerToTriggerBaseEdges(op, prevOp->opPtr, opId,
                        prevOp->opId, opAsync, prevAsync);
                    updated = true;
                  break;
                  default: LOGE("state %d of service %s encountered spurious previous state %d", state, serviceName.c_str(), prevOp->opPtr->arg1);
                     gDvm.isRunABC = false;
                     return updated;
                }
    
                //update datastructure
                if(updated){
                    //update bound service state machine
                    service->prevBoundServiceOp->opId = opId;
                    service->prevBoundServiceOp->opPtr = op;

                    //since we have seen unbindService we can see bindService now
                    service->bindServiceOp = -1;
                    service->validRequestUnbindSet.clear();
                    service->firstBindServiceRequest = -1;

                    //clear this service entry in each connection that was associated with this service
                    std::set<u4>::iterator connIt = service->connectionSet.begin();
                    for( ; connIt != service->connectionSet.end(); ++connIt){
                        bindIter = AbcServiceConnectMap.find(identifier);
                        if(bindIter != AbcServiceConnectMap.end()){
                            AbcRequestBind* tmpArb = bindIter->second;
                            AbcRequestBind* tmpArbAhead = NULL;
                            do{
                                if(tmpArb->serviceClassname == serviceName){
                                    if(tmpArbAhead == NULL){
                                        if(tmpArb->prev == NULL){
                                            AbcServiceConnectMap.erase(identifier);
                                            delete tmpArb;
                                            break;
                                        }else{
                                            bindIter->second = tmpArb->prev;
                                            delete tmpArb;
                                            break;
                                        }
                                    }else{
                                        tmpArbAhead->prev = tmpArb->prev;
                                        delete tmpArb;
                                        break;
                                    }
                                }
                                tmpArbAhead = tmpArb;
                                tmpArb = tmpArb->prev;
                            }while(tmpArb != NULL);

                        }else{
                            LOGE("ABC-ABORT: missing connection entry in AbcServiceConnectMap when "
                                 "performing UNBIND-SERVICE for service %s", serviceName.c_str());
                            updated = false;
                            gDvm.isRunABC = false;
                            return updated;
                        }
                    }

                    service->connectionSet.clear();
                }

            }else{
                LOGE("ABC-ABORT: UNBIND-SERVICE seen without a prior ServiceMap entry"
                     " or with no existing BIND-SERVICE call, for service %s", serviceName.c_str());
                gDvm.isRunABC = false;
                return updated;
            }

        }
        else if(state == ABC_STOP_SERVICE){
            if(serIter != AbcServiceMap.end() && serIter->second->createServiceOp != -1){
                AbcService* service = serIter->second;
                AbcAsync* opAsync = getAsyncBlockFromId(op->asyncId);
                if(opAsync == NULL){
                    LOGE("ABC-ABORT: missing async block for STOP-SERVICE of service: %s", serviceName.c_str());
                    gDvm.isRunABC = false;
                    return updated;
                }

                //add edge from previous state to current one
                AbcOpWithId* prevBoundOp = service->prevBoundServiceOp;
                AbcOpWithId* prevStartedOp = service->prevStartedServiceOp;
                bool correctState = false;
                switch(prevBoundOp->opPtr->arg1){
                  case ABC_CREATE_SERVICE: 
                  case ABC_UNBIND_SERVICE: correctState = true;
                }

                switch(prevStartedOp->opPtr->arg1){
                  case ABC_CREATE_SERVICE:
                  case ABC_SERVICE_ARGS: correctState = true;
                }

                //atleast one of bind-ser or service-args should have happened before stopService
                if(correctState && (prevBoundOp->opPtr->arg1 != ABC_CREATE_SERVICE ||
                           prevStartedOp->opPtr->arg1 != ABC_CREATE_SERVICE)){
                    AbcAsync* prevBoundAsync = getAsyncBlockFromId(prevBoundOp->opPtr->asyncId);
                    addTriggerToTriggerBaseEdges(op, prevBoundOp->opPtr, opId,
                        prevBoundOp->opId, opAsync, prevBoundAsync);

                    AbcAsync* prevStartedAsync = getAsyncBlockFromId(prevStartedOp->opPtr->asyncId);
                    addTriggerToTriggerBaseEdges(op, prevStartedOp->opPtr, opId,
                        prevStartedOp->opId, opAsync, prevStartedAsync);


                    if(prevStartedOp->opPtr->arg1 != ABC_CREATE_SERVICE){
                        if(service->firstStopServiceRequest != -1){
                            addEdgeToHBGraph(service->firstStopServiceRequest, opId);
                            addEdgeToHBGraph(service->firstStopServiceRequest, opAsync->postId);
                        }else{
                            LOGE("ABC-ABORT: STOP-SERVICE seen on started service %s without"
                                 " a stopService or stopSelf request", serviceName.c_str());
                            gDvm.isRunABC = false;
                            return updated;
                        }
                    }

                    updated = true;

                    free(service->prevStartedServiceOp);
                    free(service->prevBoundServiceOp);
                    delete service;
                    AbcServiceMap.erase(serviceName);
                }else{
                    LOGE("state %d of service %s encountered spurious previous states "
                         "started: %d and bound:%d", state, serviceName.c_str(), prevStartedOp->opPtr->arg1, prevBoundOp->opPtr->arg1);
                    gDvm.isRunABC = false;
                    return updated;
                }

            }else{
                LOGE("ABC-ABORT: STOP-SERVICE seen without a prior ServiceMap entry"
                     " or with no existing CREATE-SERVICE call, for service %s", serviceName.c_str());
                gDvm.isRunABC = false;
                return updated;
            } 

        }
        if(state == ABC_SERVICE_ARGS){
            if(serIter != AbcServiceMap.end() && serIter->second->createServiceOp != -1){
                AbcService* service = serIter->second;
                AbcAsync* opAsync = getAsyncBlockFromId(op->asyncId);
                if(opAsync == NULL){
                    LOGE("ABC-ABORT: missing async block for SERVICE-ARGS of service: %s", serviceName.c_str());
                    gDvm.isRunABC = false;
                    return updated;
                }

                //add edge startService request to this op connected by intentID
                std::map<u4, AbcOpWithId*>::iterator startIter = AbcRequestStartServiceMap.find(identifier);
                if(startIter != AbcRequestStartServiceMap.end()){
                    addEdgeToHBGraph(startIter->second->opId, opId);
                    addEdgeToHBGraph(startIter->second->opId, opAsync->postId);

                    //clear entry from AbcRequestStartServiceMap as wont be used again
                    AbcOpWithId* tmpPtr = startIter->second;
                    AbcRequestStartServiceMap.erase(identifier);
                    free(tmpPtr);
                }else{
                    LOGE("ABC-ABORT: onStartCommand seen without corresponding startService");
                    gDvm.isRunABC = false;
                    return updated;
                }

                //add edge from previous state to current one
                AbcOpWithId* prevOp = service->prevStartedServiceOp;
                AbcAsync* prevAsync;
                switch(prevOp->opPtr->arg1){
                  case ABC_CREATE_SERVICE:
                  case ABC_SERVICE_ARGS:
                    prevAsync = getAsyncBlockFromId(prevOp->opPtr->asyncId);
                    addTriggerToTriggerBaseEdges(op, prevOp->opPtr, opId,
                        prevOp->opId, opAsync, prevAsync);
                    updated = true;
                  break;
                  default: LOGE("state %d of service %s encountered spurious previous state %d", state, serviceName.c_str(), prevOp->opPtr->arg1);
                     gDvm.isRunABC = false;
                     return updated;
                }

                if(updated){
                    service->prevStartedServiceOp->opId = opId;
                    service->prevStartedServiceOp->opPtr = op;
                }

            }else{
                LOGE("ABC-ABORT: onStartCommand seen without a prior ServiceMap entry"
                     " or with no existing CREATE-SERVICE call, for service %s", serviceName.c_str());
                gDvm.isRunABC = false;
                return updated;
            }  

        }
        else if(state == ABC_CONNECT_SERVICE){
            if(serIter != AbcServiceMap.end() && serIter->second->bindServiceOp != -1){
                AbcService* service = serIter->second;
                AbcAsync* opAsync = getAsyncBlockFromId(op->asyncId);
                if(opAsync == NULL){
                    LOGE("ABC-ABORT: missing async block for CONNECT-SERVICE of service: %s", serviceName.c_str());
                    return updated;
                }

                //add edge from bindService to onServiceConnected
                //from code it is clear that onServiceConnected is executed only after BIND-SERVICE
                addEdgeToHBGraph(service->bindServiceOp, opId);
                addEdgeToHBGraph(service->bindServiceOp, opAsync->postId);

                //add edge from request-bind to onServiceConnected
                bindIter = AbcServiceConnectMap.find(identifier);
                if(bindIter != AbcServiceConnectMap.end()){
                    AbcRequestBind* tmpArb = bindIter->second;
                    do{
                        if(serviceName == tmpArb->serviceClassname){
                            if(tmpArb->requestBindOp->opId < opAsync->postId){
                                addEdgeToHBGraph(tmpArb->requestBindOp->opId, opId);
                                addEdgeToHBGraph(tmpArb->requestBindOp->opId, opAsync->postId);
                            }//else onServiceConnected seen is corresponding to a request which has been unbound
                             //our modelling cannot capture all possible states for service accurately but 
                             //can work in most cases
                            break;
                        }
                        tmpArb = tmpArb->prev;  
                    }while(tmpArb != NULL);
                }else{
                    LOGE("ABC-MISSING: missing bindService request");
                }

                updated = true;
            }else{
                AbcAsync* opAsync = getAsyncBlockFromId(op->asyncId);
                if(opAsync == NULL){
                    LOGE("ABC-ABORT: missing async block for CONNECT-SERVICE of service: %s", serviceName.c_str());
                    return updated;
                }
            //    LOGE("ABC-ABORT: calling onServiceConnected without bindService on %s", serviceName.c_str());
            //    updated = false;
                //add edge from request-bind to onServiceConnected
                bindIter = AbcServiceConnectMap.find(identifier);
                if(bindIter != AbcServiceConnectMap.end()){
                    AbcRequestBind* tmpArb = bindIter->second;
                    do{
                        if(serviceName == tmpArb->serviceClassname){
                            if(tmpArb->requestBindOp->opId < opAsync->postId){
                                addEdgeToHBGraph(tmpArb->requestBindOp->opId, opId);
                                addEdgeToHBGraph(tmpArb->requestBindOp->opId, opAsync->postId);
                            }//else onServiceConnected seen is corresponding to a request which has been unbound
                             //our modelling cannot capture all possible states for service accurately but 
                             //can work in most cases
                            break;
                        }
                        tmpArb = tmpArb->prev;
                    }while(tmpArb != NULL);
                }else{
                    LOGE("ABC-MISSING: missing bindService request");
                }

                updated = true;
            }
        }
        else if(state == ABC_REQUEST_STOP_SERVICE){
            if(serIter != AbcServiceMap.end()){
                if(serIter->second->firstStopServiceRequest == -1){
                    serIter->second->firstStopServiceRequest = opId;
                }
            }
            updated = true;
            
        }
    }else{
        //because, REQUEST_UNBIND_SERVICE only gets connnection obj as input
        std::map<u4, AbcRequestBind*>::iterator connIter =  
                AbcServiceConnectMap.find(identifier);
        if(connIter != AbcServiceConnectMap.end()){
            //for all services using this connection update their requestUnbindOp value
            AbcRequestBind* tmpArb = connIter->second;
            
            do{
                //ignore if unbind requests are made after a connection is unbound
                if(tmpArb->requestUnbindOp == -1){
                    //no HB edges will be added now. This info is used only on seeing a UNBIND-SER
                    tmpArb->requestUnbindOp = opId;
                    serviceName.assign(tmpArb->serviceClassname);
                    std::map<std::string, AbcService*>::iterator serIter = AbcServiceMap.find(serviceName);
                    if(serIter != AbcServiceMap.end()){
                        serIter->second->validRequestUnbindSet.insert(opId);
                    }else{
                        LOGE("ABC-ABORT: service state machine not created for %s even though bindService seen."
                             "something wrong in ABC code",serviceName.c_str());
                        gDvm.isRunABC = false;
                        return updated;
                    }
                }
                
                tmpArb = tmpArb->prev;
            }while(tmpArb != NULL);

            updated = true;
        }else{
            //calling unbind without prior bind might lead to illegalState exception
            //but calling unbind first wont affect our state machine
            updated = true; //unbind-request need not come only on active connection. Ignore it and comtinue
        }
    }

    return updated;
}

bool abcMapInstanceWithIntentId(u4 instance, int intentId){
    bool shouldAbort = false;
    AbcInstanceIntentMap.insert(std::make_pair(instance, intentId));
    //change key from intentId to instance in all maps, now that you have activity instance info

    if(ActivityStateMap.find(intentId) != ActivityStateMap.end()){
        AbcOpWithId* tmpOp = ActivityStateMap.find(intentId)->second;
        ActivityStateMap.erase(intentId);
        ActivityStateMap.insert(std::make_pair(instance, tmpOp));
    }else{
        gDvm.isRunABC = false;
        shouldAbort = true;
        LOGE("ABC-MISSING: Activity state machine error. instance %d supplied without "
             "intentId %d entry in stateMap", instance, intentId);
        return shouldAbort;
    }

    //for LAUNCH and RESUME enable may be issued with intent id. Reassign it to instance
    int state = ABC_LAUNCH;
    std::pair<u4, int> intentStatePair = std::make_pair(intentId, state);
    std::map<std::pair<u4, int>, AbcEnableTriggerList*>::iterator it =
            AbcEnableTriggerLcMap.find(intentStatePair);
    if(it != AbcEnableTriggerLcMap.end()){
        AbcEnableTriggerList* tmpLst = it->second;
        AbcEnableTriggerLcMap.erase(intentStatePair);

        std::pair<u4, int> instanceStatePair = std::make_pair(instance, state);
        AbcEnableTriggerLcMap.insert(std::make_pair(instanceStatePair, tmpLst));
    }

    state = ABC_RESUME;
    intentStatePair = std::make_pair(intentId, state);
    it = AbcEnableTriggerLcMap.find(intentStatePair);
    if(it != AbcEnableTriggerLcMap.end()){
        AbcEnableTriggerList* tmpLst = it->second;
        AbcEnableTriggerLcMap.erase(intentStatePair);

        std::pair<u4, int> instanceStatePair = std::make_pair(instance, state);
        AbcEnableTriggerLcMap.insert(std::make_pair(instanceStatePair, tmpLst));
    }


    return shouldAbort;
}

void checkAndAddToMapIfActivityResult(int opId, AbcOp* op){
    if(op->arg1 == ABC_RESULT){
        AbcActivityResult* ar;
        if(ActivityResultMap.find(op->arg2->id) != ActivityResultMap.end()){
            ar = ActivityResultMap.find(op->arg2->id)->second;
            if(ar->enable2 == NULL){
                ar->enable2 = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
                ar->enable2->opId = opId;
                ar->enable2->opPtr = op;
            }else{
                ar->enable2->opId = opId;
                ar->enable2->opPtr = op;
            }
        }else{
            ar = (AbcActivityResult*)malloc(sizeof(AbcActivityResult));
            ar->enable2 = NULL;
            ar->enable1 = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
            ar->enable1->opId = opId;
            ar->enable1->opPtr = op;
            ActivityResultMap.insert(std::make_pair(op->arg2->id, ar));
        }
    }
}

void addEnableLifecycleEventToMap(int opId, AbcOp* op){
    u4 instance = op->arg2->id;
    int state = op->arg1;
    std::pair<u4, int> instanceStatePair = std::make_pair(instance, state);
    std::map<std::pair<u4, int>, AbcEnableTriggerList*>::iterator it =
            AbcEnableTriggerLcMap.find(instanceStatePair);

    if(it != AbcEnableTriggerLcMap.end()){
        AbcEnableTriggerList* lst = (AbcEnableTriggerList*)malloc(sizeof(AbcEnableTriggerList));
        lst->enable = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
        lst->trigger = NULL;
        lst->enable->opId = opId;
        lst->enable->opPtr = op;
        lst->prev = it->second;
        it->second = lst;
    }else{
        AbcEnableTriggerList* lst = (AbcEnableTriggerList*)malloc(sizeof(AbcEnableTriggerList));
        lst->enable = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
        lst->trigger = NULL;
        lst->enable->opId = opId;
        lst->enable->opPtr = op;
        lst->prev = NULL;
        
        AbcEnableTriggerLcMap.insert(std::make_pair(instanceStatePair, lst));
    }
}

//add edges: ENABLE -> TRIGGER and ENABLE -> postOf(TRIGGER)
//ENABLE to callOf(TRIGGER) should get derived conditional-transitively
bool connectEnableAndTriggerLifecycleEvents(int triggerOpid, AbcOp* triggerOp){

    u4 instance = triggerOp->arg2->id;
    int state = triggerOp->arg1;
    std::pair<u4, int> instanceStatePair = std::make_pair(instance, state);
    std::map<std::pair<u4, int>, AbcEnableTriggerList*>::iterator it = 
            AbcEnableTriggerLcMap.find(instanceStatePair);
    
  
    bool edgeAdded = false;
    if(it != AbcEnableTriggerLcMap.end()){
        AbcEnableTriggerList* etList = it->second;
        //connect trigger to all previous continous enable which dont have corresponding trigger
        do{
            if(etList->trigger != NULL){
                break;
            }else{
                etList->trigger = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
                etList->trigger->opId = triggerOpid;
                etList->trigger->opPtr = triggerOp;
            }
            edgeAdded = true;
            addEdgeToHBGraph(etList->enable->opId, triggerOpid);
            //add edge from enableOpid to trigger's asyncblock's postId
            //In case of Timer task this wont be there..hence the check
            if(triggerOp->asyncId != -1){
                AbcAsync* triggerAsync = getAsyncBlockFromId(triggerOp->asyncId);
                if(triggerAsync != NULL){
                    addEdgeToHBGraph(etList->enable->opId, triggerAsync->postId);
                }
            }
            etList = etList->prev;
        }while(etList != NULL);
    }

    //if the operation is RESUME then add edge from existing blank enable to this 
    //if this resume has not yet been connected to an enable. 
    //Note: we dont have a better way of doing this than with a guess work
    if(triggerOp->arg1 == ABC_RESUME){
        if(blankEnableResumeOp->opId != -1 && !edgeAdded){
            addEdgeToHBGraph(blankEnableResumeOp->opId, triggerOpid);
            AbcAsync* async = getAsyncBlockFromId(triggerOp->asyncId);
            if(async != NULL){
                addEdgeToHBGraph(blankEnableResumeOp->opId, async->postId);
            }
            edgeAdded = true;
            //add this enable-trigger to enableTriggerMap
            if(it != AbcEnableTriggerLcMap.end()){
                AbcEnableTriggerList* lst = (AbcEnableTriggerList*)malloc(sizeof(AbcEnableTriggerList));
                lst->enable = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
                lst->trigger = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
                lst->enable->opId = blankEnableResumeOp->opId;
                lst->enable->opPtr = blankEnableResumeOp->opPtr;
                lst->trigger->opId = triggerOpid;
                lst->trigger->opPtr = triggerOp;
                lst->prev = it->second;
                it->second = lst;
            }

            blankEnableResumeOp->opId = -1;
            blankEnableResumeOp->opPtr = NULL;
        }
    }

    return edgeAdded;
}

bool isEventActivityEvent(int eventId){
    bool isActivityEvent = false;
    switch(eventId){
      case ABC_LAUNCH:
      case ABC_RESUME:
      case ABC_ACTIVITY_START:
      case ABC_PAUSE:
      case ABC_STOP:
      case ABC_DESTROY:
      case ABC_RESULT:
      case ABC_NEW_INTENT:
           isActivityEvent = true;
           break;
      default: isActivityEvent = false;
    }

    return isActivityEvent;
}

/*method acting as Activity state machine and also update 
 *component state in ActivityStateMap
 */
bool checkAndUpdateComponentState(int opId, AbcOp* op){
    bool updated = false;
    AbcOpWithId* prevOperation = NULL;
    u4 instance = op->arg2->id;
    int state = op->arg1;
    AbcAsync* curOpAsync = NULL; 
    AbcAsync* prevOpAsync = NULL;

    if(state != ABC_LAUNCH){
        if(ActivityStateMap.find(instance) != ActivityStateMap.end()){
            prevOperation = ActivityStateMap.find(instance)->second;

            curOpAsync = getAsyncBlockFromId(op->asyncId);
            prevOpAsync = getAsyncBlockFromId(prevOperation->opPtr->asyncId);
        }else{
            gDvm.isRunABC = false;
            LOGE("ABC-MISSING: Activity state machine error. state %d seen before instantiation", state);
            return updated;
        }
    }

    /*in all the states except LAUNCH check if prevOperation of the activity is in 
     *the expected state, only then perform other task
     */
    switch(state){
      case ABC_LAUNCH:
        //if any instance with same id remaining clear it
        //should not happen usually. but not considering as a serious error...
        ActivityStateMap.erase(instance); 
 
        //add an entry to activity state tracking map
        /*for LAUNCH instance is actually intentId. Instance will be sent later
         *and this will be rewritten
         */
        prevOperation = (AbcOpWithId*)malloc(sizeof(AbcOpWithId));
        ActivityStateMap.insert(std::make_pair(instance, prevOperation));
        updated = true;
        break;
      case ABC_RESUME:
        switch(prevOperation->opPtr->arg1){
            case ABC_LAUNCH:
            case ABC_ACTIVITY_START:
            case ABC_PAUSE:
            case ABC_STOP:
            case ABC_NEW_INTENT:
            case ABC_RESULT: 
              addTriggerToTriggerBaseEdges(op, prevOperation->opPtr, opId,
                  prevOperation->opId, curOpAsync, prevOpAsync);
              updated = true;
              break;
            default: LOGE("state %d of instance %d encountered spurious previous state %d", state, instance, prevOperation->opPtr->arg1);
                     gDvm.isRunABC = false;
                     updated = false;
        } 
        break;	  
      case ABC_ACTIVITY_START:
        switch(prevOperation->opPtr->arg1){
            case ABC_STOP: 
              addTriggerToTriggerBaseEdges(op, prevOperation->opPtr, opId,
                  prevOperation->opId, curOpAsync, prevOpAsync);
              updated = true;
              break;
            default: LOGE("state %d of instance %d encountered spurious previous state %d", state, instance, prevOperation->opPtr->arg1);
                     gDvm.isRunABC = false;
                     updated = false;
        }
        break;
      case ABC_PAUSE:
        switch(prevOperation->opPtr->arg1){
            case ABC_RESUME:
              addTriggerToTriggerBaseEdges(op, prevOperation->opPtr, opId,
                  prevOperation->opId, curOpAsync, prevOpAsync);
              updated = true;
              break;
            default: LOGE("state %d of instance %d encountered spurious previous state %d", state, instance, prevOperation->opPtr->arg1);
                     gDvm.isRunABC = false;
                     updated = false;
        }
        break;
      case ABC_STOP:
        switch(prevOperation->opPtr->arg1){
            case ABC_PAUSE:
              addTriggerToTriggerBaseEdges(op, prevOperation->opPtr, opId,
                  prevOperation->opId, curOpAsync, prevOpAsync);
              updated = true;
              break;
            default: LOGE("state %d of instance %d encountered spurious previous state %d", state, instance, prevOperation->opPtr->arg1);
                     gDvm.isRunABC = false;
                     updated = false;
        }
        break;
      case ABC_DESTROY:
        switch(prevOperation->opPtr->arg1){
            case ABC_STOP:
              addTriggerToTriggerBaseEdges(op, prevOperation->opPtr, opId,
                  prevOperation->opId, curOpAsync, prevOpAsync);
              updated = true;
              break;
            default: LOGE("state %d of instance %d encountered spurious previous state %d", state, instance, prevOperation->opPtr->arg1);
                     gDvm.isRunABC = false;
                     updated = false;
        }
        break;
      case ABC_RESULT:
        switch(prevOperation->opPtr->arg1){
            case ABC_STOP:
            case ABC_NEW_INTENT:
              addTriggerToTriggerBaseEdges(op, prevOperation->opPtr, opId,
                  prevOperation->opId, curOpAsync, prevOpAsync);
              updated = true;
              break;
            default: LOGE("state %d of instance %d encountered spurious previous state %d", state, instance, prevOperation->opPtr->arg1);
                     gDvm.isRunABC = false;
                     updated = false;
        }
        break;
      case ABC_NEW_INTENT:
        switch(prevOperation->opPtr->arg1){
            case ABC_RESULT:
            case ABC_PAUSE:
            case ABC_STOP:
              addTriggerToTriggerBaseEdges(op, prevOperation->opPtr, opId,
                  prevOperation->opId, curOpAsync, prevOpAsync);
              updated = true;
              break;
            default: LOGE("state %d of instance %d encountered spurious previous state %d", state, instance, prevOperation->opPtr->arg1);
                     gDvm.isRunABC = false;
                     updated = false;
        }
        break;
    /*  case ABC_START_NEW_INTENT:
        updated = true;
        break;
      case ABC_RELAUNCH:
        updated = true;
        break;*/
    } 

    //update information of activity state
    if(state != ABC_DESTROY){
        prevOperation->opId = opId;
        prevOperation->opPtr = op;
    }else{
        ActivityStateMap.erase(instance);
        free(prevOperation);
    }

    return updated;
}
