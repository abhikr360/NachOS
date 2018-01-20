// stats.h 
//	Data structures for gathering statistics about Nachos performance.
//
// DO NOT CHANGE -- these stats are maintained by the machine emulation
//
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef STATS_H
#define STATS_H

#include "copyright.h"

// The following class defines the statistics that are to be kept
// about Nachos behavior -- how much time (ticks) elapsed, how
// many user instructions executed, etc.
//
// The fields in this class are public to make it easier to update.

class Statistics {
  public:
    int totalTicks;      	// Total time running Nachos
    int idleTicks;       	// Time spent idle (no threads to run)
    int systemTicks;	 	// Time spent executing system code
    int userTicks;       	// Time spent executing user code
				// (this is also equal to # of
				// user instructions executed)

    int countOverallBurst;//-- total number of CPU bursts
    int overallCpuBurst;//-- total time in CPU bursts
    int minCpuBurst;//-- maximum of all cpu bursts
    int maxCpuBurst;//-- minimum of all cpu bursts

    int TimerTicks;

    int overallWait;//-- total time spent in waiting
    float averageWait; // -- average time spent waiting

    int totalTimeInThread;//-- sum of totalTime of all the threads
    int numThread;//-- number of threads
    float avgThread; // - average execution time of thread
    int longestThread;// -- longest execution time over all threads
    int shortestThread;//-- shortest execution time over all threads



    int numDiskReads;		// number of disk read requests
    int numDiskWrites;		// number of disk write requests
    int numConsoleCharsRead;	// number of characters read from the keyboard
    int numConsoleCharsWritten; // number of characters written to the display
    int numPageFaults;		// number of virtual memory page faults
    int numPacketsSent;		// number of packets sent over the network
    int numPacketsRecvd;	// number of packets received over the network

    
    double predError; //-- this variable is for calculating the error in our prediction of next cpu burst while implementing SJF
    int totalProcessEndTime;// -- sum of end time of all processes
    long long squareSumProcessEndTime;  // -- square of sum of end time of all processes
    float avgProcessEndTime; // -- average of end time of all processes
    int minProcessEndTime; // -- min of end time of all processes
    int maxProcessEndTime; // -- max of end time of all processes
    


    Statistics(); 		// initialize everything to zero

    void Print();		// print collected statistics
};

// Constants used to reflect the relative time an operation would
// take in a real system.  A "tick" is a just a unit of time -- if you 
// like, a microsecond.
//
// Since Nachos kernel code is directly executed, and the time spent
// in the kernel measured by the number of calls to enable interrupts,
// these time constants are none too exact.

#define UserTick 	1	// advance for each user-level instruction 
#define SystemTick 	10 	// advance each time interrupts are enabled
#define RotationTime 	500 	// time disk takes to rotate one sector
#define SeekTime 	500    	// time disk takes to seek past one track
#define ConsoleTime 	100	// time to read or write one character
#define NetworkTime 	100   	// time to send or receive one packet
// #define TimerTicks 100   	// (average) time between timer interrupts

#endif // STATS_H
