// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void 
PrintAllPageTable()
{
	for (int i = 0; i < ThreadMaxNum; i++) {
		Thread* t = ThreadArray[i];
		if (t != NULL && t->space != NULL) {
			printf("thread %s valid page num: %d\n", t->getName(), t->space->validPageCount());
		}
	}
		
}

void execFunc(int address) {
	char name[10];
	int pos = 0;
	int data;
	OpenFile *executable;
	AddrSpace *space;

	while (1) {
		machine->ReadMem(address+pos, 1, &data);
		if (data == 0){
			name[pos] = '\0';
			break;
		}
		name[pos++] = char(data);
	}
	
	executable = fileSystem->Open(name);
	space = new AddrSpace(executable);
	currentThread->space = space;
	delete executable;
	space->InitRegisters();
	space->RestoreState();
	machine->Run();
}

void forkFunc(int address)
{
/*	Info* info = (Info*)address;
	AddrSpace *space = info->space;

	currentThread->space = space;
	space->InitRegister();
	space->RestoreState();
	machine->WriteRegister(PCReg, info->pc);
	machine->WriteRegister(NextPCReg, info->pc+4);
	machine->Run();
*/
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    } else if ((which == SyscallException) && (type == SC_Create)) {
		printf("syscall create\n");
		int address = machine->ReadRegister(4);
		char name[10];
		int pos = 0;
		int data;

		while (1) {
			machine->ReadMem(address+pos, 1, &data);
			if (data == 0) {
				name[pos] = '\0';
				break;
			}
			name[pos++] = char(data);
		}

		fileSystem->Create(name, 128);

		machine->PCAdvance();
	} else if ((which == SyscallException) && (type == SC_Open)) {
		printf("syscall open\n");
		int address = machine->ReadRegister(4);
		char name[10];
		int pos = 0;
		int data;

		while (1) {
			machine->ReadMem(address + pos, 1, &data);
			if (data == 0) {
				name[pos] = '\0';
				break;
			}
			name[pos++] = char(data);
		}

		OpenFile *file = fileSystem->Open(name);
		machine->WriteRegister(2, int(file));

		machine->PCAdvance();
	} else if ((which == SyscallException) && (type == SC_Close)) {
		printf("syscall close\n");
		int fd = machine->ReadRegister(4);
		OpenFile *file = (OpenFile*)fd;
		delete file;

		machine->PCAdvance();

	} else if ((which == SyscallException) && (type == SC_Read)) {
		printf("syscall read\n");
		int pos = machine->ReadRegister(4);
		int size = machine->ReadRegister(5);
		int fd = machine->ReadRegister(6);
		char content[size];
		int result;
		OpenFile *file = (OpenFile*)fd;

		result = file->Read(content, size);
		for (int i = 0; i < result; i++) {
			machine->WriteMem(pos+i, 1, int(content[i]));
		}
		machine->WriteRegister(2, result);

		machine->PCAdvance();
	} else if ((which == SyscallException) && (type == SC_Write)) {
		printf("syscall write\n");
		int pos = machine->ReadRegister(4);
		int size = machine->ReadRegister(5);
		int fd = machine->ReadRegister(6);
		char content[size];
		int data;
		OpenFile *file = (OpenFile*)fd;

		for (int i = 0; i < size; i++) {
			machine->ReadMem(pos+i, 1, &data);
			content[i] = char(data);
		}
		file->Write(content, size);
		machine->PCAdvance();
	} else if ((which == SyscallException) && (type == SC_Exec)) {
		printf("syscall exec\n");
		int address = machine->ReadRegister(4);
		Thread* thread = new Thread("thread", 1);
		thread->Fork(execFunc, address);
		machine->WriteRegister(2, thread->gettid());

		machine->PCAdvance();
	} else if ((which == SyscallException) && (type == SC_Fork)) {
		printf("syscall fork\n");
/*		int funcPC = machine->ReadRegister(4);
		OpenFile *executable = fileSystem->Open(currentThread->filename);
		AddrSpace *space = new AddrSpace(executable);
		space->addrspaceCp(currentThread->space);
		Info* info = new Info;
		info->space = space;
		info->pc = funcPC;

		Thread *thread = new Thread("thread");
		thread->Fork(forkFunc, int(info));
*/
		machine->PCAdvance();
	} else if ((which == SyscallException) && (type == SC_Yield)) {
		printf("syscall yield\n");
		machine->PCAdvance();
		currentThread->Yield();
	} else if ((which == SyscallException) && (type == SC_Join)) {
		printf("syscall join\n");
		int id = machine->ReadRegister(4);
		while (ThreadArray[id])
			currentThread->Yield();
		machine->PCAdvance();
	} else if ((which == SyscallException) && (type == SC_Exit)) {
		printf("syscall exit\n");
		int status = machine->ReadRegister(4);
		
//		machine->clear();
		machine->PCAdvance();
		currentThread->Finish();
	}
	else if (which == PageFaultException) {
        
		// tlb miss
        int virtAddr = machine->ReadRegister(BadVAddrReg);
        unsigned int vpn = (unsigned) virtAddr / PageSize;
		int pos;
		
        printf("handling tlb miss, addr %d!\n", virtAddr);
		printf("\nthread %s is running!\n",currentThread->getName());
		PrintAllThread();
		PrintAllPageTable();
		printf("\n");


		//fifo
		if (machine->tlbStrategy == 0) {
			pos = machine->tlbReplacePos;
			machine->tlbReplacePos = (pos + 1) % TLBSize;
		}
		else {
			// NRU
        	pos = machine->nruQueue->Remove();
			machine->nruQueue->Prepend(pos);
		}

		// real page fault
		if (machine->pageTable[vpn].valid == 0) {
			printf("physical page is not in the memory!\n");

			int physPage = machine->getPhysicalPage(currentThread->space, vpn);
			machine->pageTable[vpn].physicalPage = physPage;
			machine->pageTable[vpn].valid = 1;					
			machine->pageTable[vpn].dirty = 0;
			//machine->pageTable[vpn].virtualPage = vpn;
			printf("loading new physical page %d!\n", physPage);
		}


		printf("add virtual page %d, physical page %d to tlb[%d]\n", vpn, machine->pageTable[vpn].physicalPage, pos);
        machine->tlb[pos] = machine->pageTable[vpn];               // save the page entry to tlb[0]
	
		if (machine->tlb[pos].valid == 0) {
			printf("tlb[%d] is not valid.\n", pos);
			printf("something is going wrong!\n");
		} 
	} else{
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
