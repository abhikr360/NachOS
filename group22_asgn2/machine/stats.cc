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
#include "limits.h"

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

  overallWait=0;
  averageWait=0;

  TimerTicks=100;

  totalTimeInThread=0;
  numThread=0;
  avgThread=0;
  longestThread=0;
  shortestThread=INT_MAX;
  countOverallBurst=0;
  overallCpuBurst=0;
  minCpuBurst=INT_MAX;
  maxCpuBurst=0;


  predError=0;
  totalProcessEndTime = 0;
  squareSumProcessEndTime = 0;
  avgProcessEndTime = 0;
  minProcessEndTime = INT_MAX;
  maxProcessEndTime = 0;


}

//----------------------------------------------------------------------
// Statistics::Print
// 	Print performance metrics, when we've finished everything
//	at system shutdown.
//----------------------------------------------------------------------

void
Statistics::Print()
{

 //    printf("Ticks: total %d, idle %d, system %d, user %d\n", totalTicks, 
	// idleTicks, systemTicks, userTicks);
 //    printf("Disk I/O: reads %d, writes %d\n", numDiskReads, numDiskWrites);
 //    printf("Console I/O: reads %d, writes %d\n", numConsoleCharsRead, 
	// numConsoleCharsWritten);
 //    printf("Paging: faults %d\n", numPageFaults);
 //    printf("Network I/O: packets received %d, sent %d\n", numPacketsRecvd, 
	// numPacketsSent);
    averageWait = overallWait/numThread;
    avgThread = totalTimeInThread/numThread;
    avgProcessEndTime = totalProcessEndTime/numThread;
    long long t=(long long)totalProcessEndTime;
    // printf("[][][]%lld %lld %lld %lf\n",t,(long long)squareSumProcessEndTime,((long long)squareSumProcessEndTime*10 - t*t),(double)(squareSumProcessEndTime-t*t)/(1.0*numThread));
    double varianceProcessEndTime = squareSumProcessEndTime*1.0/numThread -(long long)avgProcessEndTime*(long long)avgProcessEndTime;//(t*t)/(1.0*numThread*numThread);
    // double varianceProcessEndTime = squareSumProcessEndTime*1.0/numThread -(t*t)/(1.0*numThread*numThread);

    printf("Total CPU Busy Time  : %d \n", overallCpuBurst);
    printf("Total Time in threads: %d \n", totalTimeInThread);
    printf("Total Execution Time : %d \n", totalTicks);
    printf("CPU Utilization : %f\n", (overallCpuBurst*100.0)/totalTicks);
    printf("Maximum CPU Burst : %d, Minimum CPU Burst : %d, Average CPU Burst : %f\n",  maxCpuBurst,minCpuBurst, (1.0*overallCpuBurst)/countOverallBurst);
    printf("No. of non zero CPU Bursts Observed : %d\n", countOverallBurst);
    printf("Average Waiting Time in Ready Queue : %f\n", averageWait);
    printf("Maximum Thread Completion Time :  %d\n", maxProcessEndTime);
    printf("Minimum Thread Completion Time :  %d\n", minProcessEndTime);
    printf("Average Thread Completion Time :  %f\n", avgProcessEndTime);
    printf("Variance of Thread Completion Time :  %lf\n", varianceProcessEndTime);
    // printf("%f\n",);


    // printf("overallWait : %d  averageWait : %f\n", overallWait, averageWait);
    // printf("totalTimeInThread : %d  numThread : %d  avgThread : %f\n", totalTimeInThread, numThread, avgThread);
    // printf("countOverallBurst : %d  overallCpuBurst : %d minCpuBurst : %d maxCpuBurst : %d avgCpuBurst : %f\n", countOverallBurst, overallCpuBurst, minCpuBurst, maxCpuBurst, (1.0*overallCpuBurst)/countOverallBurst);
    // printf("Cpu utilization : %f\n", (overallCpuBurst*100.0)/totalTicks);
    // printf("totalProcessEndTime : %d avgProcessEndTime : %f minProcessEndTime : %d maxProcessEndTime  : %d\n",totalProcessEndTime, avgProcessEndTime, minProcessEndTime, maxProcessEndTime );
    // printf("longestThread : %d shortestThread : %d \n",longestThread,shortestThread );
     // printf("Prediction Error %f\n", predError);
    printf("Prediction Error : %f  \n", 100.0*predError/overallCpuBurst);
}
