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

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d tid:%d uid:%d looped %d times\n", which, currentThread->gettid(), currentThread->getuid(), num);
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
		interrupt->SetLevel(IntOff);
		interrupt->SetLevel(IntOn);
	}
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
    DEBUG('t', "Entering ThreadTest1");

    Thread *t1 = new Thread("thread1", 1);
	Thread *t2 = new Thread("thread2", 1);
	Thread *t3 = new Thread("thread3", 1);

    t1->Fork(SimpleThread, (void*)1);
	t2->Fork(SimpleThread, (void*)2);
	t3->Fork(SimpleThread, (void*)3);
    SimpleThread(0);
}

// test for 
// thread limit 128
// ./nachos -q 2
void 
ThreadLimitTest()
{
	DEBUG('t', "Entering ThreadLimitTest");

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
	DEBUG('t', "Entering ThreadTest3");
	
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
	DEBUG('t', "Entering ThreadTest4");

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
    default:
	printf("No test specified.\n");
	break;
    }
}

