-------------------------------------------------------------------README----------------------------------------------------------------------
Overview

To the Thread object the following attributes are added:
  a. childThreadList - List of children's thread pointers
  b. exitStat - Exit status of current thread. Initialized to 0.
  c. isZombie - Initially False. Changes to True on exit.
  d. waitingForChild - Stores PID of the child waiting for.
  e. parentThread - Stores  parent thread's pointer

To the Thread object the following methods are added:
  a.ExitThread -
      Deletes the current thread. Checks if there are any processes either in ready queue, sleeping list or in transition.
      If not, halts.
  b.getExitStatus - Returns exitStat
  c.setPID - Sets the PID of the process.
  d.setPPID -  Sets the PPID of the process.
  e.InsertChildThread - Inserts a thread pointer in the childThreadList.
  f.GetChildThread - Returns the pointer to the thread corresponding to a given PID from childThreadList.
  g.UpdateChildStatus - Updates the thread's childThreadList to reflect the exit status of its child.
  h.Construcor 2 - Constructor for zombie threads to store their exit status.Rest all fields NULL.

New Global variables defined
  a.MaxPages - Counts Max. no. of pages used
  b.numActiveProcesses - Used to track the number of active processes so that machine doesn't halt prematurely
  c.PID_count - Counts Max. PID assigned yet.
  d.MemoryFull - Indicates if memory is full.

In the ProcessAddrSpace class,the following changes are made:
  a.Constructor updated to copy memory not from 0 but from MaxPage .
  b.New constructor to copy data from other thread and not an executable.

System Calls

1.GetReg
            Values are read using the ReadRegister() function and values using the WriteRegister() function.
            The argument is read from register 4 and value returned in register 2.
            If argument is not a valid register number i.e not between 1-39 -1, is returned.
            Else, the required register is read and it's content returned.

2.GetPA
            Checks if the argument(i.e the virtual address) is  >=0. If not, returns -1 and exits.
            Calculates the virtual page number by dividing virtual address by page size and offset by taking the remainder.
            Checks whether the virtual page number is within bounds and that the virtual address is valid. If not, returns -1 and exits.
            Reads the physical page number from the attribute of the NachOSpageTable of the machine.
            Checks if the physical page number is within range. If not, returns -1 and exits.
            Physical Address is calculated using the page size and offset(which is same).

3,4.GetPID,GetPPID
            First we defined a public methods in the thread object to get the pid,ppid which are private attributes.
            Then we call these functions namely getPID and getPPID and return the required value.

5.GetNumInstr
              The value of the userTicks attribute of the Statistics class defined in stats.h is returned.

6.Time
              The value of the totalTicks attribute of the Statistics class defined in stats.h is returned.

7.Yield
              The YieldCPU method of the NachOSThread class is called.

8.Sleep
              A global list called the sleepingThreadList is created which keeps track of the sleeping processes.
              The argument is read into a variable. If it is 0, YieldCPU is called.
              Otherwise, the Schedule function with the time argument is called and the process is added to the sleepingThreadList.
              Then, the PutThreadToSleep is called.


9.Exec
              Creates new thread.
              Copies code into its address space. Returns -1 if MemoryFull.
              Calls ThreadFork to fork out the new process.
              Calls FinishThread to delete the currentThread and decrements numActiveProcesses


10.Exit
              First checks if the current thread has a parent.
              It calls UpdateChildStatus to update the paretn thread's childList to reflect the exit status of its child.
              Checks if parent is waiting for it and wake if necessary.
              Calls ExitThread.
11.Join
              Returns -1 if child does not exist.
              Checks if child isZombie i.e. already exited or not.
              If yes, fetches the exutstatus and returns.
              Else, places child PID in waitingForChild and sleeps.

12.Fork
              Creates new child.
              Copies parent's address space into it using Constructor 2. Returns -1 if MemoryFull.
              Sets PID,PPID,parent thread appropriately and inserts child in parent's childThreadList.
              Writes 0 in register 2, calls SaveUserState and forks it using ThreadFork.
              Returns child's PID to parent.            

-----------------------------------------------------------------------------------------------------------------------------------------------
