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
#include "console.h"
#include "synch.h"

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
static Semaphore *readAvail;
static Semaphore *writeDone;
static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

static void WakeThread(int thread){
  //function to wake a thread passed as argument from sleeping state
   NachOSThread *oldThread = (NachOSThread*)thread;
   //disable interrupts
   IntStatus oldLevel = interrupt->SetLevel(IntOff);
   //remove the thread from sleepingThreadList
   NachOSThread *newth = scheduler->RemoveSleepingThread(oldThread);

   ASSERT(newth->getPID() == oldThread->getPID());

   //put the thread in ready list
   scheduler->ThreadIsReadyToRun(oldThread);
   //retain the previous level of interrupt
   (void) interrupt->SetLevel(oldLevel);
   return;
}

//Child Thread begins executing from this function
static void ExecuteForkedChild(int arg)
{
	if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
	       threadToBeDestroyed = NULL;
    }
        currentThread->RestoreUserState();     // to restore, do it.
	      currentThread->space->RestoreStateOnSwitch();

   machine->Run();
}

//Executed process begins from this function
static void ExecProcess(int arg){
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }

	  currentThread->space->InitUserCPURegisters();
	  currentThread->space->RestoreStateOnSwitch();

    machine->Run();
}

static void ConvertIntToHex (unsigned v, Console *console)
{
   unsigned x;
   if (v == 0) return;
   ConvertIntToHex (v/16, console);
   x = v % 16;
   if (x < 10) {
      writeDone->P() ;
      console->PutChar('0'+x);
   }
   else {
      writeDone->P() ;
      console->PutChar('a'+x-10);
   }
}

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    int memval, vaddr, printval, tempval, exp;
    unsigned printvalus;        // Used for printing in hex

    if (!initializedConsoleSemaphores) {
       readAvail = new Semaphore("read avail", 0);
       writeDone = new Semaphore("write done", 1);
       initializedConsoleSemaphores = true;
    }
    Console *console = new Console(NULL, NULL, ReadAvail, WriteDone, 0);;

    if ((which == SyscallException) && (type == SYScall_Halt)) {
		DEBUG('a', "Shutdown, initiated by user program.\n");
   		interrupt->Halt();
    }
    else if ((which == SyscallException) && (type == SYScall_GetReg)) {
        //read the register number and store it in variable reg
        int reg = machine->ReadRegister(4);
        //error handling
        //Returns -1 if invalid register number
        if (reg < 0 || reg > 39){
        	machine->WriteRegister(2,-1);
        }
        //return value in register reg
        else {
            machine->WriteRegister(2,(unsigned)machine->ReadRegister(reg));
        }
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SYScall_GetPA)){
        unsigned int vpn,offset;
        unsigned int pageframe;
        //read the virtual address
        vaddr = machine->ReadRegister(4);
        //error handling
        if (vaddr < 0){
        	machine->WriteRegister(2,-1);
        }
        else {
          //calculate virtual page number and offset
          vpn = (unsigned) vaddr/PageSize;
          offset = (unsigned) vaddr%PageSize;
          //error handling
          //checks if virtual page number lies within bounds
          if(vpn >= machine->pageTableSize){
              machine->WriteRegister(2,-1);
          }
          //check if page table entry has the valid field set to true
          else if (!(machine->NachOSpageTable[vpn].valid)){
              machine->WriteRegister(2,-1);
          }
          else {
              TranslationEntry *entry;
              entry = &machine->NachOSpageTable[vpn];
              pageframe = entry->physicalPage;
              //checks if obtained physical page number is not larger than the number of physical pages
              if (pageframe >= NumPhysPages){
                  machine->WriteRegister(2,-1);
              }
              //calculate physical address and return
              else machine->WriteRegister(2,pageframe*PageSize + offset);
          }
        }
        // Advance program counters.
        machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }

    // for getPID and getPPID we define public methods in thread object to get those values
    else if ((which == SyscallException) && (type == SYScall_GetPID)) {
      //get pid from the method defined in currentThread
      machine->WriteRegister(2, currentThread->getPID());

      // Advance program counters.
      machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
      machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
      machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SYScall_GetPPID)) {
      //get ppid from the method defined in currentThread
      machine->WriteRegister(2, currentThread->getPPID());

      // Advance program counters.
      machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
      machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
      machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SYScall_Time)){
      //read  total ticks at present from stats
       machine->WriteRegister(2,stats->totalTicks);
        // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SYScall_NumInstr)){
      //read userticks from stats
       machine->WriteRegister(2,stats->userTicks);
        // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SYScall_Sleep)){

       int ticks = machine->ReadRegister(4);
        // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
       //if ticks=0 no need to sleep. Just call Yeild
       if(ticks == 0){
          currentThread->YieldCPU();
       }
       else if(ticks > 0 ){
         //schedule to wakeup the thread after given interval
          interrupt->Schedule(&(WakeThread), (int)currentThread, ticks, TimerInt);
          IntStatus oldLevel = interrupt->SetLevel(IntOff);
          //insert currentThread in sleepingThreadList
          scheduler->InsertSleepingThread(currentThread);
          //put hte current running thread to sleep
          currentThread->PutThreadToSleep();
          //retain the interrupt level
          (void) interrupt->SetLevel(oldLevel);
       }
    }
    else if ((which == SyscallException) && (type == SYScall_Yield)) {
      
	// Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
	//call the predefined method in thread.cc
       currentThread->YieldCPU();
    }
    else if ((which == SyscallException) && (type == SYScall_Fork)) {
      //Initialize a new thread with name child
       NachOSThread *childThread = new NachOSThread("child");
       //Copy parent's address space into it
       childThread->space = new ProcessAddrSpace();
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

       if(MemoryFull == TRUE)
       {
          numActiveProcesses--;
          machine->WriteRegister(2, -1);
          delete childThread;
          return;
       }

       machine->WriteRegister(2, 0);
       //set pid and ppid of childThread
       childThread->setPPID(currentThread->getPID());
       childThread->parentThread = currentThread;
       //insert child thread in the childThreadList of parentThread
       currentThread->InsertChildThread(childThread);
       //save cpu state of childThread
       childThread->SaveUserState();
       //execute forked childThread and return its pid
       childThread->ThreadFork(&(ExecuteForkedChild), 1);
       machine->WriteRegister(2, childThread->getPID());
    }
    else if ((which == SyscallException) && (type == SYScall_Join)) {

       int pid = machine->ReadRegister(4);
       NachOSThread* myChild = currentThread->GetChildThread(pid);
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

       if(myChild == NULL)    //error handling if child with required pid does not exist
       {
            machine->WriteRegister(2,-1);
       }
       else
       {
           if(myChild->isZombie == TRUE)  //if thread with given pid alread exited then return its exit status
                machine->WriteRegister(2, myChild->getExitStatus());
           else
           {    //thread with given pid exist
                //set waitingForChild of current thread as pid of thread to be exited
                currentThread->waitingForChild = pid;
                IntStatus oldLevel = interrupt->SetLevel(IntOff);
                //put the thread to sleep
                currentThread->PutThreadToSleep();
                //retain the previous level of interrupt
                (void) interrupt->SetLevel(oldLevel);
                NachOSThread *ch = currentThread->GetChildThread(pid);
                if(ch == NULL)
                  machine->WriteRegister(2,-1);
                  //error handling if child with required pid does not exist
                else
                  machine->WriteRegister(2, ch->getExitStatus());
                  //return the exit status of required
           }
       }
    }
    else if ((which == SyscallException) && (type == SYScall_Exec)) {
      //create new thread and assign pid to it
      NachOSThread* execThread = new NachOSThread("exec");
      execThread->setPID(currentThread->getPID());
      PID_count--;
	    char filename[250];

	    int size = 0;

	    vaddr =  machine->ReadRegister(4);
      machine->ReadMem(vaddr, 1, (int *)(&filename[size]));   // Error Handling of  vaddr
    	while (filename[size] != '\0') {
          //reading filename
          vaddr++;
	        size++;
	        machine->ReadMem(vaddr, 1,(int *)(&filename[size]));
      }

	    OpenFile *executable = fileSystem->Open(filename);       //open the file to execute
      if (executable == NULL)
      {   //if executable is null means nothing to execute so decrease
          //the number of active process delete the thread and return -1
          numActiveProcesses--;
          delete execThread;
          machine->WriteRegister(2,-1);
          // Advance program counters.
          machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
          machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
          machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
          return;
      }

      //	Create an address space, load the program from a file "executable", and set everything
      execThread->space = new ProcessAddrSpace(executable);

      if(MemoryFull == TRUE)
      {   //error handling in case when Memory is Full
          printf("Error: No more space available\n");
          numActiveProcesses--;
          delete execThread;
          machine->WriteRegister(2,-1);
          // Advance program counters.
          machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
          machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
          machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
          return;
      }

      execThread->setPPID(currentThread->getPPID());
      delete executable;			                         // close file
      numActiveProcesses--;
      //exexute the forked thread
	    execThread->ThreadFork(&(ExecProcess),1);
	    currentThread->FinishThread();

    }
    else if ((which == SyscallException) && (type == SYScall_Exit)) {

       currentThread->waitingForChild = 0;
       int exit_status = machine->ReadRegister(4);
       NachOSThread *parent = currentThread->parentThread;
       if(parent != NULL)
       {
          int i = parent->UpdateChildStatus(currentThread->getPID(), exit_status);
          if(i!=-1 && parent->waitingForChild == currentThread->getPID())            // Wake Parent
          {
               IntStatus oldLevel = interrupt->SetLevel(IntOff);
               scheduler->ThreadIsReadyToRun(parent);
               // ThreadIsReadyToRun assumes that interrupts
					     // are disabled!
               (void) interrupt->SetLevel(oldLevel);

          }
       }
       numActiveProcesses--;
       currentThread->ExitThread();
    }
    else if ((which == SyscallException) && (type == SYScall_PrintInt)) {
       printval = machine->ReadRegister(4);
       if (printval == 0) {
	  writeDone->P() ;
          console->PutChar('0');
       }
       else {
          if (printval < 0) {
	     writeDone->P() ;
             console->PutChar('-');
             printval = -printval;
          }
          tempval = printval;
          exp=1;
          while (tempval != 0) {
             tempval = tempval/10;
             exp = exp*10;
          }
          exp = exp/10;
          while (exp > 0) {
	     writeDone->P() ;
             console->PutChar('0'+(printval/exp));
             printval = printval % exp;
             exp = exp/10;
          }
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SYScall_PrintChar)) {
	writeDone->P() ;
        console->PutChar(machine->ReadRegister(4));   // echo it!
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SYScall_PrintString)) {
       vaddr = machine->ReadRegister(4);
       machine->ReadMem(vaddr, 1, &memval);
       while ((*(char*)&memval) != '\0') {
	  writeDone->P() ;
          console->PutChar(*(char*)&memval);
          vaddr++;
          machine->ReadMem(vaddr, 1, &memval);
       }
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    }
    else if ((which == SyscallException) && (type == SYScall_PrintIntHex)) {
       printvalus = (unsigned)machine->ReadRegister(4);
       writeDone->P() ;
       console->PutChar('0');
       writeDone->P() ;
       console->PutChar('x');
       if (printvalus == 0) {
          writeDone->P() ;
          console->PutChar('0');
       }
       else {
          ConvertIntToHex (printvalus, console);
        }
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}

