#include "Wqueue.h"
#include "Dalvik.h"
list<WorkItem*> queue;
void wqueueInit() {
	gDvm.q = new wqueue<WorkItem*>(queue);
}

