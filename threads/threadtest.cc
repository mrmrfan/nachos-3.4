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
SimpleThread1(int which) {}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t1 = new Thread("thread1");
	Thread *t2 = new Thread("thread2");
	Thread *t3 = new Thread("thread3");

    t1->Fork(SimpleThread, (void*)1);
	t2->Fork(SimpleThread, (void*)2);
	t3->Fork(SimpleThread, (void*)3);
    SimpleThread(0);
}

void 
ThreadLimitTest()
{
	DEBUG('t', "Entering ThreadLimitTest");

	for (int i = 0; i < 128; i++) {
		Thread *t = new Thread("");
		printf("create thread %d tid:%d uid:%d\n", i, t->gettid(), t->getuid());
	}
}

void 
ThreadTest3()
{
	DEBUG('t', "Entering ThreadTest3");
	
	PrintAllThread();

	for (int i = 0; i < 5; i++) {
		Thread *t = new Thread("  ");
		t->Fork(SimpleThread1, (void*)i);
	}

	PrintAllThread();
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
    default:
	printf("No test specified.\n");
	break;
    }
}

