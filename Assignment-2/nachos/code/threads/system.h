// system.h
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"

#define MAX_THREAD_COUNT 1000
#define BASE_PRIORITY 50
#define INIT_GUESS 20
#define SJP_RATIO 0.5

// Initialization and cleanup routines
extern void Initialize(int argc, char **argv); 	// Initialization,
						// called before anything else
extern void Cleanup();				// Cleanup, called when
						// Nachos is done.

extern NachOSThread *currentThread;			// the thread holding the CPU
extern NachOSThread *threadToBeDestroyed;  		// the thread that just finished
extern NachOSscheduler *scheduler;			// the ready list
extern Interrupt *interrupt;			// interrupt status
extern Statistics *stats;			// performance metrics
extern Timer *timer;				// the hardware alarm clock
extern unsigned numPagesAllocated;              // number of physical frames allocated

extern NachOSThread *threadArray[];  			// Array of thread pointers
extern unsigned thread_index;                  // Index into this array (also used to assign unique pid)
extern bool initializedConsoleSemaphores;       // Used to initialize the semaphores for console I/O exactly once
extern bool exitThreadArray[];          // Marks exited threads


//-----------------Defined By me------------
extern unsigned TimerTicks;            // Quanta for preemptive schedAlgo
extern bool doYield;                   // Used to ensure burst completion
extern int schedAlgo;                  // Scheduling Algorithm used
extern unsigned threadPriority[];		   // Thread priority array
extern unsigned threadBasePriority[];  // Thread base priority array
extern unsigned threadCpuCount[];      // Thread CPU count
extern unsigned burstPredict[];
extern unsigned prevBurst[];

/*extern unsigned TotalCPUbusytime ;
extern unsigned Totalexecutiontime;
extern unsigned MaximumBurstLength ;
extern unsigned MinimumBurstLength ;
extern unsigned NumBursts;
extern unsigned NumProcesses;
extern unsigned MaximumExecutionTime ;
extern unsigned MinimumExecutionTime ;
extern unsigned TotalWaittime;
*/


//------------------------------------------

class TimeSortedWaitQueue {             // Needed to implement SYScall_Sleep
private:
   NachOSThread *t;                           // Thread pointer of the sleeping thread
   unsigned when;                       // When to wake up
   TimeSortedWaitQueue *next;           // Build the list

public:
   TimeSortedWaitQueue (NachOSThread *th,unsigned w) { t = th; when = w; next = NULL; }
   ~TimeSortedWaitQueue (void) {}

   NachOSThread *GetThread (void) { return t; }
   unsigned GetWhen (void) { return when; }
   TimeSortedWaitQueue *GetNext(void) { return next; }
   void SetNext (TimeSortedWaitQueue *n) { next = n; }
};

extern TimeSortedWaitQueue *sleepQueueHead;

#ifdef USER_PROGRAM
#include "machine.h"
extern Machine* machine;	// user program memory and registers
#endif

#ifdef FILESYS_NEEDED 		// FILESYS or FILESYS_STUB
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;
#endif

#endif // SYSTEM_H
