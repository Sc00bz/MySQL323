CC=g++
CFLAGS64=-c -m64 -Wall -O2
CFLAGS32=-c -m32 -Wall -O2
LFLAGS64=-lpthread -m64
LFLAGS32=-lpthread -m32

all: mysql323-64 mysql323-32
	

# 64 bit
mysql323-64: main64.o atomic64.o common64.o keyspace64.o mysql323keyspace64.o mysql323thread64.o thread64.o workqueue64.o
	$(CC) $(LFLAGS64) -o mysql323-64 main64.o atomic64.o common64.o keyspace64.o mysql323keyspace64.o mysql323thread64.o thread64.o workqueue64.o

atomic64.o: atomic.cpp atomic.h common.h architecture.h thread.h
	$(CC) $(CFLAGS64) -o atomic64.o atomic.cpp

common64.o: common.cpp common.h architecture.h
	$(CC) $(CFLAGS64) -o common64.o common.cpp

keyspace64.o: keyspace.cpp keyspace.h common.h architecture.h
	$(CC) $(CFLAGS64) -o keyspace64.o keyspace.cpp

mysql323keyspace64.o: mysql323keyspace.cpp mysql323keyspace.h common.h architecture.h keyspace.h
	$(CC) $(CFLAGS64) -o mysql323keyspace64.o mysql323keyspace.cpp

mysql323thread64.o: mysql323thread.cpp mysql323thread.h common.h architecture.h atomic.h thread.h workqueue.h mysql323keyspace.h keyspace.h
	$(CC) $(CFLAGS64) -o mysql323thread64.o mysql323thread.cpp

thread64.o: thread.cpp thread.h
	$(CC) $(CFLAGS64) -o thread64.o thread.cpp

workqueue64.o: workqueue.cpp workqueue.h common.h architecture.h thread.h
	$(CC) $(CFLAGS64) -o workqueue64.o workqueue.cpp

main64.o: main.cpp atomic.h common.h architecture.h thread.h mysql323thread.h workqueue.h keyspace.h
	$(CC) $(CFLAGS64) -o main64.o main.cpp

# 32 bit
mysql323-32: main32.o atomic32.o common32.o keyspace32.o mysql323keyspace32.o mysql323thread32.o thread32.o workqueue32.o
	$(CC) $(LFLAGS32) -o mysql323-32 main32.o atomic32.o common32.o keyspace32.o mysql323keyspace32.o mysql323thread32.o thread32.o workqueue32.o

atomic32.o: atomic.cpp atomic.h common.h architecture.h thread.h
	$(CC) $(CFLAGS32) -o atomic32.o atomic.cpp

common32.o: common.cpp common.h architecture.h
	$(CC) $(CFLAGS32) -o common32.o common.cpp

keyspace32.o: keyspace.cpp keyspace.h common.h architecture.h
	$(CC) $(CFLAGS32) -o keyspace32.o keyspace.cpp

mysql323keyspace32.o: mysql323keyspace.cpp mysql323keyspace.h common.h architecture.h keyspace.h
	$(CC) $(CFLAGS32) -o mysql323keyspace32.o mysql323keyspace.cpp

mysql323thread32.o: mysql323thread.cpp mysql323thread.h common.h architecture.h atomic.h thread.h workqueue.h mysql323keyspace.h keyspace.h
	$(CC) $(CFLAGS32) -o mysql323thread32.o mysql323thread.cpp

thread32.o: thread.cpp thread.h
	$(CC) $(CFLAGS32) -o thread32.o thread.cpp

workqueue32.o: workqueue.cpp workqueue.h common.h architecture.h thread.h
	$(CC) $(CFLAGS32) -o workqueue32.o workqueue.cpp

main32.o: main.cpp atomic.h common.h architecture.h thread.h mysql323thread.h workqueue.h keyspace.h
	$(CC) $(CFLAGS32) -o main32.o main.cpp

clean:
	-rm *.o mysql323-64 mysql323-32
