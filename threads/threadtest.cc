// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "elevatortest.h"
#include "synch.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void clock_tick()
{
	interrupt->SetLevel(IntOff);
	interrupt->SetLevel(IntOn);
}

void
SimpleThread()
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread tid:%d uid:%d looped %d times\n", currentThread->gettid(), currentThread->getuid(), num);
        currentThread->Yield();
    }
}

void
SimpleThread1(int which) 
{
	if (currentThread->getprior() == 0)
		currentThread->Yield();
	printf("*** thread %d tid:%d uid:%d prior:%d\n", which, currentThread->gettid(), currentThread->getuid(), currentThread->getprior());
}

void 
SimpleThread2(int which)
{
	for (int i = 0; i < 10*which; i++) {
		printf("thread tid:%d running in prior:%d with %d time slices\n", currentThread->gettid(), currentThread->getprior(), currentThread->gettime_slices());
		clock_tick();
	}
}

struct args2
{
	int which;
	int N;
	int* cnt;
	Semaphore* S;
	Lock* mutex;
	Condition* barrier;
	args2() {
		cnt = NULL;
		S = NULL;
		mutex = NULL;
		barrier = NULL;
	}
};


// designed for barrier 
void 
SimpleThread3(args2* arg)
{
	int which = arg->which;
	int N = arg->N;
	int* cnt = arg->cnt;
	Semaphore* S = arg->S;
	Lock* mutex = arg->mutex;
	Condition* barrier = arg->barrier;

	for (int i = 0; i < which; i++) {
		S->P();
		printf("thread tid:%d enters place 1\n", currentThread->gettid());
		mutex->Acquire();
        printf("thread tid:%d enters place 2\n", currentThread->gettid());
		*cnt = *cnt + 1;
		if (*cnt < N)
			barrier->Wait(mutex);
		else
			barrier->Broadcast(mutex);
		
		// critical area
		printf("thread tid:%d passing!\n", currentThread->gettid());

		*cnt = *cnt - 1;
		if (*cnt == 0) {
			for (int j = 0 ; j < N; j++)
				S->V();
		}
		mutex->Release();
		printf("thread tid:%d exits\n", currentThread->gettid());

	}
}

// for semaphore
struct args
{
	int which;
	int* buffer;
	int n;
	Semaphore* full;
	Semaphore* empty;

	args(int _which, void* _buffer, int _n, void* _full, void* _empty) {
		which = _which;
		buffer = (int*)_buffer;
		n = _n;
		full = (Semaphore*)_full;
		empty = (Semaphore*)_empty;
	}

};

// for condition var
struct args1
{
	int which;
	int* buffer;
	int n;
	int* cnt;
	Lock* mutex;
	Condition* full;
	Condition* empty;

	args1() {
		cnt = NULL;
		buffer = NULL;
		mutex = NULL;
		full = NULL;
		empty = NULL;
	}
};

// based on Semaphore
void
Producer0(args* arg) 
{
	int which = arg->which;
	int* buffer = arg->buffer;
	int n = arg->n;
	Semaphore* full = arg->full;
	Semaphore* empty = arg->empty;

	for (int i = 0; i < which; i++)	{
		clock_tick();
		empty->P();
		clock_tick();
		// produce
		for (int j = 0; j < n; j++) {
			clock_tick();	
			if (buffer[j] == -1) { 
				printf("Producer %d produces item %d at pos %d\n", i, i, j);
				buffer[j] = i;
				break;
			}
		}
		clock_tick();
		full->V();
		clock_tick();
	}
}

// based on Semaphore
void 
Consumer0(args* arg)
{
	int which = arg->which;
	int* buffer = arg->buffer;
	int n = arg->n;
	Semaphore* full = arg->full;
	Semaphore* empty = arg->empty;

	for (int i = 0; i < which; i++) {
		clock_tick();
		full->P();
		clock_tick();
		
		//consume
		for (int j = 0; j < n; j++) {
			clock_tick();
			if (buffer[j] != -1) {
				printf("Consumer %d consumes item %d at pos %d\n", i, buffer[j], j);
				buffer[j] = -1;
				break;
			}
		}
		clock_tick();
		empty->V();
		clock_tick();
	}
}


// based on condition var
void 
Producer1(args1* arg) 
{
	int which = arg->which;
    int* buffer = arg->buffer;
    int n = arg->n;
    int* cnt = arg->cnt;
    Condition* full = arg->full;
    Condition* empty = arg->empty;
    Lock* mutex = arg->mutex;

	for (int i = 0; i < which; i++) {
		clock_tick();
		mutex->Acquire();
		while (*cnt == n) {
			empty->Wait(mutex);
		}
		clock_tick();
	
		//produce
		for (int j = 0; j < n; j++) {
        	clock_tick();
	   	 	if (buffer[j] == -1) {
			    printf("Producer %d produces item %d at pos %d\n", i, i, j);
		   	 	buffer[j] = i;
		   	 	break;
			}
		}
		*cnt = *cnt + 1;

		clock_tick();
		full->Signal(mutex);
		mutex->Release();
		clock_tick();
	}
}

// based on condition var
void Consumer1(args1* arg)
{
	int which = arg->which;
	int* buffer = arg->buffer;
	int n = arg->n;
	int* cnt = arg->cnt;
	Condition* full = arg->full;
	Condition* empty = arg->empty;
	Lock* mutex = arg->mutex;

	for (int i = 0; i < which; i++) {
		clock_tick();
		mutex->Acquire();
		while (*cnt == 0) {
			full->Wait(mutex);
		}
		clock_tick();
		
		// consume
		for (int j = 0; j < n; j++) {
    		clock_tick();
       		if (buffer[j] != -1) {
	    		printf("Consumer %d consumes item %d at pos %d\n", i, buffer[j], j);
	        	buffer[j] = -1;
		        break;
		    }
		}
		*cnt = *cnt - 1;

		clock_tick();
		empty->Signal(mutex);
		mutex->Release();
		clock_tick();
	}
}

// args for rw_lock
struct args3
{
	int which;
	int* cnt;
	Lock* mutex;
	Lock* rw_lock;
	args3(){
		cnt = NULL;
		mutex = NULL;
		rw_lock = NULL;
	}
};

// rw_lock
void 
Reader(args3* arg)
{
	int which = arg->which;
	int* cnt = arg->cnt;
	Lock* mutex = arg->mutex;
	Lock* rw_lock = arg->rw_lock;
	
	clock_tick();
	clock_tick();
	mutex->Acquire();
	*cnt = *cnt + 1;
	if (*cnt == 1)
		rw_lock->Acquire();
	mutex->Release();
	clock_tick();
	clock_tick();

	// read
	printf("reader %d is reading!\n", which);

	clock_tick();
	clock_tick();
	mutex->Acquire();
	*cnt = *cnt - 1;
	if (*cnt == 0)
		rw_lock->Release();
	mutex->Release();
	clock_tick();
	clock_tick();
}

// rw_lock
void 
Writer(args3* arg)
{
	int which = arg->which;
	Lock* rw_lock = arg->rw_lock;
	
	clock_tick();
	rw_lock->Acquire();
	clock_tick();

	// write
	printf("writer %d is writing!\n", which);
	
	clock_tick();
	rw_lock->Release();
	clock_tick();
}


//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------


// test for 
// printing tid & uid and so on
// ./nachos
void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1\n");

    Thread *t1 = new Thread("thread1", 1);
//	Thread *t2 = new Thread("thread2", 1);
//	Thread *t3 = new Thread("thread3", 1);

    t1->Fork(SimpleThread, NULL);
//	t2->Fork(SimpleThread);
//	t3->Fork(SimpleThread);
    SimpleThread();
}

// test for 
// thread limit 128
// ./nachos -q 2
void 
ThreadLimitTest()
{
	DEBUG('t', "Entering ThreadLimitTest\n");

	for (int i = 0; i < 128; i++) {
		Thread *t = new Thread("", 1);
		printf("create thread %d tid:%d uid:%d\n", i, t->gettid(), t->getuid());
	}
}

// test for 
// the TS instr
// ./nachos -q 3    or ./nachos -TS
void 
ThreadTest3()
{
	DEBUG('t', "Entering ThreadTest3\n");
	
	PrintAllThread();

	for (int i = 0; i < 5; i++) {
		Thread *t = new Thread("  ", 1);
		t->Fork(SimpleThread1, (void*)i);
	}

	PrintAllThread();
}

// test for 
// priority-based + preemptible
// test example: main=1   0432104321
// ./nachos -q 4
void 
ThreadTest4()
{
	DEBUG('t', "Entering ThreadTest4\n");

/*
	for (int i = 0; i < 5; i++) {
		Thread *t1 = new Thread(" ", (5-i)%5);
		Thread *t2 = new Thread(" ", (5-i)%5);
		
		printf("create thread %d tid:%d uid:%d prior:%d\n", 2*i, t1->gettid(), t1->getuid(), t1->getprior());
		t1->Fork(SimpleThread1, (void*)(2*i));
		printf("create thread %d tid:%d uid:%d prior:%d\n", 2*i+1, t2->gettid(), t2->getuid(), t2->getprior());
		t2->Fork(SimpleThread1, (void*)(2*i+1));
	}
*/
	for (int i = 0; i < 5; i++) {
	    Thread *t = new Thread(" ", (5-i)%5);
		printf("create thread %d tid:%d uid:%d prior:%d\n", i, t->gettid(), t->getuid(), t->getprior());
		t->Fork(SimpleThread1, (void*)(i)); 
	}
	for (int i = 0; i < 5; i++) {
	    Thread *t = new Thread(" ", (5-i)%5);
		printf("create thread %d tid:%d uid:%d prior:%d\n", i+5, t->gettid(), t->getuid(), t->getprior());
		t->Fork(SimpleThread1, (void*)(i+5));     
	}


}

// test for 
// RR algorithm
// ./nachos -rs 0 -d + -q 5
void
ThreadTest5()
{
	Thread *t1 = new Thread("t1", 1);
	Thread *t2 = new Thread("t2", 1);
	
	printf("create thread tid:%d uid:%d prior:%d\n", t1->gettid(), t1->getuid(), t1->getprior());
	t1->Fork(SimpleThread2, (void*)1);
	printf("create thread tid:%d uid:%d prior:%d\n", t2->gettid(), t2->getuid(), t2->getprior());
	t2->Fork(SimpleThread2, (void*)2);
}

// test for 
// multi-level feedback queue scheduling algorithm
// set the timeticks to 40
// ./nachos -rs 0 -q 6
void 
ThreadTest6()
{
	Thread *t1 = new Thread("t1", 1);
	t1->Fork(SimpleThread2, (void*)2);
    printf("create thread tid:%d uid:%d prior:%d\n", t1->gettid(), t1->getuid(), t1->getprior());

	Thread *t2 = new Thread("t2", 1);
    t2->Fork(SimpleThread2, (void*)4);
    printf("create thread tid:%d uid:%d prior:%d\n", t2->gettid(), t2->getuid(), t2->getprior());

	Thread *t3 = new Thread("t3", 1);
    t3->Fork(SimpleThread2, (void*)3);
    printf("create thread tid:%d uid:%d prior:%d\n", t3->gettid(), t3->getuid(), t3->getprior());


	Thread *t4 = new Thread("t4", 1);
    t4->Fork(SimpleThread2, (void*)5);
    printf("create thread tid:%d uid:%d prior:%d\n", t4->gettid(), t4->getuid(), t4->getprior());

}

// producer-consumer   Semaphore
// ./nachos -rs 0/1/2 -q 7
void
Producer2Consumer0()
{
	Thread* pro = new Thread("producer", 1);
	Thread* con = new Thread("consumer", 1);
	args* arg;
	int n = 3;  // or 4 or 5 or 6
	int* buffer = new int[n];
	Semaphore* full = new Semaphore("full", 0);
	Semaphore* empty = new Semaphore("empty", n);

	for (int i = 0; i < n; i++) 
		buffer[i] = -1;

	arg = new args(10, (void*)buffer, n, (void*)full, (void*)empty);
	
	pro->Fork(Producer0, (void*)arg);
	printf("start Producer!\n");
	con->Fork(Consumer0, (void*)arg);
	printf("start Consumer!\n");
}
	
// Producer-Consumer   condition var
// ./nachos -rs 0/1/2 -q 8
void 
Producer2Consumer1() 
{
	Thread* pro = new Thread("producer", 1);
    Thread* con = new Thread("consumer", 1);
    args1* arg;
	int n = 3;
	int* buffer = new int[n];
	int cnt = 0;
	Lock* mutex = new Lock("mutex");
	Condition* full = new Condition("full");
	Condition* empty = new Condition("empty");

	for (int i = 0; i < n; i++)
		buffer[i] = -1;
	
	arg = new args1();
	arg->which = 10;
	arg->n = n;
	arg->cnt = &cnt;
	arg->buffer = buffer;
	arg->mutex = mutex;
	arg->full = full;
	arg->empty = empty;

	pro->Fork(Producer1, (void*)arg);
	printf("Start Producer!\n");
	con->Fork(Consumer1, (void*)arg);
	printf("Start Consumer!\n");
}

// barrier 
// ./nachos -rs 0/1/2 -q 9
void 
Barrier()
{
	int n = 5;
	int cnt = 0;
	int N = 3;
	args2* arg = new args2();
	Semaphore* S = new Semaphore("S", N);
	Lock* mutex = new Lock("mutex");
	Condition* barrier = new Condition("barrier");

	arg->which = 3;
	arg->N = N;
	arg->cnt = &cnt;
	arg->S = S;
	arg->mutex = mutex;
	arg->barrier = barrier;
		
	for (int i = 0; i < n; i++) {
        Thread* t = new Thread("t", 1);
		t->Fork(SimpleThread3, (void*)arg);
		printf("Start thread tid:%d\n", t->gettid());
	}
}

// rw_lock
// ./nachos -rs 0/1/2 -q 10
void 
RW_Lock()
{
	IntStatus oldlevel = interrupt->SetLevel(IntOff);

	int n = 5;
	int cnt = 0;
	Lock* mutex = new Lock("mutex");
	Lock* rw_lock = new Lock("rw_lock");

	for (int i = 0; i < n; i++) {
	    Thread* t1 = new Thread("t1", 1);		        
		Thread* t2 = new Thread("t2", 1);

		args3* arg = new args3();

	    arg->mutex = mutex;
	    arg->rw_lock = rw_lock;
	    arg->cnt = &cnt;
		arg->which = i;

		t1->Fork(Writer, (void*)arg);
		printf("Start Writer %d!\n", i);
		
		t2->Fork(Reader, (void*)arg);
        printf("Start Reader %d!\n", i);
	}

	(void)interrupt->SetLevel(oldlevel);
}


//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------
void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
	case 2:
	ThreadLimitTest();
	break;
	case 3:
	ThreadTest3();
	break;
	case 4:
	ThreadTest4();
	break;
	case 5:
	ThreadTest5();
	break;
	case 6:
	ThreadTest6();
	break;
	case 7:
	Producer2Consumer0();
	break;
	case 8:
	Producer2Consumer1();
	break;
	case 9:
	Barrier();
	break;
	case 10:
	RW_Lock();
	break;
    default:
	printf("No test specified.\n");
	break;
    }
}

