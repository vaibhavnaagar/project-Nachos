// stats.h
//	Routines for managing statistics about Nachos performance.
//
// DO NOT CHANGE -- these stats are maintained by the machine emulation.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "stats.h"

//----------------------------------------------------------------------
// Statistics::Statistics
// 	Initialize performance metrics to zero, at system startup.
//----------------------------------------------------------------------

Statistics::Statistics()
{
    totalTicks = idleTicks = systemTicks = userTicks = 0;
    numDiskReads = numDiskWrites = 0;
    numConsoleCharsRead = numConsoleCharsWritten = 0;
    numPageFaults = numPacketsSent = numPacketsRecvd = 0;

     TotalCPUbusytime  = 0;
     Totalexecutiontime = 0;
     MaximumBurstLength = 0;
     MinimumBurstLength = 10000000000;
     NumBursts = 0;
     NumProcesses = 0;
     MaximumExecutionTime = 0;
     MinimumExecutionTime = 10000000000;
     TotalWaittime = 0;
     EstimationError =0;

     int i;
     for(i=0; i<1000;i++)
     	ThreadExecutionArray[i] = 0;
}

//----------------------------------------------------------------------
// Statistics::Print
// 	Print performance metrics, when we've finished everything
//	at system shutdown.
//----------------------------------------------------------------------

void
Statistics::Print()
{
  if(NumProcesses == 0){
    printf("Stats may not be correct when processes are not submitted in batch(i.e. using -F) because stat of PID=0 is not included.\n");
    printf("To get stat of single process use: put it in a file and use -F option\n");
    NumProcesses = 1; // To avoid divide by zero exception
    Totalexecutiontime = 1; // again to avoid divide by zero exception
    NumBursts = 1; // again to avoid divide by zero exception
    TotalCPUbusytime = 1; // again to avoid divide by zero exception
  }
    int avg = Totalexecutiontime/NumProcesses;

   printf("Ticks: total %d, idle %d, system %d, user %d\n", totalTicks,
	idleTicks, systemTicks, userTicks);
    printf("Disk I/O: reads %d, writes %d\n", numDiskReads, numDiskWrites);
    printf("Console I/O: reads %d, writes %d\n", numConsoleCharsRead,
	numConsoleCharsWritten);
    printf("Paging: faults %d\n", numPageFaults);
    printf("Network I/O: packets received %d, sent %d\n", numPacketsRecvd,
	numPacketsSent);

    printf("Total Number of Processes= %d\n", NumProcesses);
    printf("Total CPU Burst Time : %d\n",TotalCPUbusytime);
    printf("Total Execution time : %d\n",Totalexecutiontime);
    printf("CPU Utilization :%f\n",(100*TotalCPUbusytime)/(float)Totalexecutiontime);
   printf("CPU BURST: Max. CPU burst is :%d, Min. CPU burst is :%d, Avg. CPU burst is :%d and Number of bursts is:%d\n",MaximumBurstLength,MinimumBurstLength,TotalCPUbusytime/NumBursts,NumBursts);
    printf("Average Waiting Time is :%d\n",TotalWaittime/NumProcesses);

    int i, total=0;
     for(i=0; i<NumProcesses;i++)
        total += (ThreadExecutionArray[i] - avg)*(ThreadExecutionArray[i] - avg);

     printf("Completion Time: Maximum= %d  Minimum= %d Average= %d  Variance= %d\n",MaximumExecutionTime, MinimumExecutionTime, avg, total/NumProcesses);
    printf(" Estimation error  is :%f\n",(float)((float)EstimationError/TotalCPUbusytime));

}
