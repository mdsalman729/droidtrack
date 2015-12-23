/*
 * Copyright (C) 2007 The Android Open Source Project
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

package java.lang;

class VMThread {
    Thread thread;
    int vmData;

    VMThread(Thread t) {
        thread = t;
    }

    native static void create(Thread t, long stackSize);

    static native Thread currentThread();
    static native boolean interrupted();
    static native void sleep (long msec, int nsec) throws InterruptedException;
    static native void yield();

    native void interrupt();

    native boolean isInterrupted();

    /**
     *  Starts the VMThread (and thus the Java Thread) with the given
     *  stack size.
     */
    void start(long stackSize) {
        VMThread.create(thread, stackSize);
    }

    /**
     * Queries whether this Thread holds a monitor lock on the
     * given object.
     */
    native boolean holdsLock(Object object);

    native void setPriority(int newPriority);
    native int getStatus();

    /**
     * Holds a mapping from native Thread statuses to Java one. Required for
     * translating back the result of getStatus().
     */
    static final Thread.State[] STATE_MAP = new Thread.State[] {
        Thread.State.TERMINATED,     // ZOMBIE
        Thread.State.RUNNABLE,       // RUNNING
        Thread.State.TIMED_WAITING,  // TIMED_WAIT
        Thread.State.BLOCKED,        // MONITOR
        Thread.State.WAITING,        // WAIT
        Thread.State.NEW,            // INITIALIZING
        Thread.State.NEW,            // STARTING
        Thread.State.RUNNABLE,       // NATIVE
        Thread.State.WAITING,        // VMWAIT
        Thread.State.RUNNABLE        // SUSPENDED
    };

    /**
     * Tell the VM that the thread's name has changed.  This is useful for
     * DDMS, which would otherwise be oblivious to Thread.setName calls.
     */
    native void nameChanged(String newName);
    
    /*Android bug-checker*/
    native void abcPrintPostMsg(int msgCode, long delay, int isFrontPost);
    
    native void abcPrintCallMsg(int msgCode);
    
    native void abcPrintRetMsg(int msgCode);
    
    native void abcPrintAttachQueue(int queueHash);
    
    native void abcPrintLoop(int queueHash);
    
    native void abcLogAddIdleHandler(int idleHandlerHash, int queueHash);
    
    native void abcLogRemoveIdleHandler(int idleHandlerHash, int queueHash);
    
    native void abcLogQueueIdle(int idleHandlerHash, int queueHash);
    
    native void abcPrintRemoveMsg(int msg);
    
    native void abcSendDbAccessInfo(String dbPath, int dbAction);
    
    native void abcForceAddEnableEvent(int viewHash, int eventCode);
	
	native void abcRemoveAllEventsOfView(int viewHash, int ignoreEvent);
	
	native void abcAddEnableEventForView(int viewHash, int eventType);
	
	native void abcTriggerEvent(int viewHash, int eventType);
	
	native void abcEnableWindowFocusChangeEvent(int windowHash);
	
	native void abcTriggerWindowFocusChangeEvent(int windowHash);
	
	native int abcPerformRaceDetection();
	
	native void abcEnableLifecycleEvent(String component, int compId, int status);
	
	native void abcTriggerLifecycleEvent(String component, int compId, int status);
	
	native void abcTriggerServiceLifecycle(String component, int compId, int status);
	
//	native void abcRegisterBroadcastReceiver(String component, String action);
//	
//	native void abcTriggerBroadcastReceiver(String component, String action, int triggerNow);
	
	native void abcTriggerBroadcastLifecycle(String action, int componentId, int intentId, int status);
	
	native void abcStopTraceGeneration();
	
	native int abcGetTraceLength();
	
	native int abcGetThreadCount();
	
	native int abcGetMessageQueueCount();
	
	native int abcGetAsyncBlockCount();
	
	native int abcGetEventTriggerCount();
	
	native int abcGetFieldCount();
	
	native int abcGetMultiThreadedRaceCount();
	
	native int abcGetAsyncRaceCount();
	
	native int abcGetDelayPostRaceCount();
	
	native int abcGetCrossPostRaceCount();
	
	native int abcGetCoEnabledEventUiRaces();
	
	native int abcGetCoEnabledEventNonUiRaces();
	
	native void abcPrintRacesDetectedToFile();
	
	native void abcComputeMemoryUsedByRaceDetector();
	
	native void abcMapInstanceWithIntentId(int instance, int intentId);
	
    /*Android bug-checker*/
}
