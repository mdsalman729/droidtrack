CC=gcc
CXX=g++
#CFLAGS=-c -O3 -g #-Wall 
CFLAGS=-c -O3
LDFLAGS=-lpthread
#SOURCES=main.c droidtrack.c ds/ds_list.c ds/ds_hash.c ds/ds_queue.c
CPPSOURCES= main.cpp droidtrack.cpp ds/ds_list.cpp ds/ds_hash.cpp ds/ds_queue.cpp 
OBJECTS=$(SOURCES:.c=.o)
CPPOBJECTS=$(CPPSOURCES:.cpp=.o)
EXECUTABLE=droidtrack

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(CPPOBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
.cpp.o:
	$(CXX) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE)
