// progtest.cc
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"


extern void ForkStartFunction(int dummy);
//----------------------------------------------------------------------
// StartUserProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void
StartUserProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);
    ProcessAddrSpace *space;

    if (executable == NULL) {
	printf("Unable to open file %s\n", filename);
	return;
    }
    space = new ProcessAddrSpace(executable);
    currentThread->space = space;
    delete executable;			// close file

    space->InitUserCPURegisters();		// set the initial register values
    space->RestoreStateOnSwitch();		// load page table register

    machine->Run();			// jump to the user progam
    printf("Main: %d\n",currentThread->GetPID());
    ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void
ConsoleTest (char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);

    for (;;) {
	readAvail->P();		// wait for character to arrive
	ch = console->GetChar();
	console->PutChar(ch);	// echo it!
	writeDone->P() ;        // wait for write to finish
	if (ch == 'q') return;  // if q, quit
    }
}

//----------------------------------------------------------------------
// CreateThreads
// Reads a file line by line and create threads and
// set priorities to each thread enqueues it in the ready queue.
// Base priority = 100 if priority is not provided in the file for a thread
//----------------------------------------------------------------------
void CreateThreads(FILE *fp)
{
    ASSERT(fp!=NULL);
    char filename[1000];
    int priority = 100;
    int basePriority = 0;
    // schedAlgo = 7 {Unix Scheduler}
    if(schedAlgo >= 7)
      basePriority = BASE_PRIORITY;

    while(fscanf(fp, "%s %d", filename, &priority) != EOF)
    {
      	OpenFile *executable = fileSystem->Open(filename);
      	if(executable != NULL)
      	{
      	    NachOSThread *newThread = new NachOSThread(filename);
      	    newThread->space = new ProcessAddrSpace(executable);
            newThread->space->InitUserCPURegisters();
      	    delete executable;

      	    threadBasePriority[newThread->GetPID()] = priority + basePriority;
            threadPriority[newThread->GetPID()] = threadBasePriority[newThread->GetPID()];

            newThread->SaveUserState ();                               // Duplicate the register set
            //newThread->ResetReturnValue ();                           // Sets the return register to zero
            newThread->AllocateThreadStack (ForkStartFunction, 0);     // Make it ready for a later context switch
      	    newThread->Schedule();
      	}

      	bzero(filename, sizeof(filename));
      	priority = 100;
    }
}
