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

//----------------------------------------------------------------------
// LaunchUserProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------


void
BatchStarter(int x){
    scheduler->Tail();
    machine->Run();
}
void
LaunchBatchProcess(char *filename)
{
    FILE *file = fopen(filename,"r" );
    if(file == NULL){
        printf("Unable to open inputfile %s\n", filename);
    }
    //printf("Here\n");
    
    char * line =NULL;

    size_t len=0;

    ssize_t read;
    int priority[100], i, flag;
    char executable[100][100];
    int k=0, count=0;
    //line stores line, read stores line's length
    while ((read = getline(&line, &len, file)) != -1) {

        flag=0;
        priority[count]=0;
        i=0;

        if(k==0){
            scheduler->schedulingAlgo = line[0]-'0';
            if(line[1] != '\n'){
                scheduler->schedulingAlgo = 10;
            }
            k++;
            continue;
        }

        while(line[i] != '\0'){
            if(line[i]==' '){
                flag=1;
                executable[count][i]='\0';
            }
            if(flag==0){
                executable[count][i] = line[i];            

            }
            if(flag==1 && line[i] <= '9' && line[i]>= '0'){
                priority[count] = 10*priority[count]+line[i]-'0';
            }
            i++;
        }
        if(flag==0){
            executable[count][i-1]='\0';
            priority[count] = 100;
        }
        // printf("currentThread %s: \n", currentThread->getName());
        // printf("executable name %d : %s\n", count, executable[count]);
        // printf("priority : %d\n", priority[count]);
        // printf("schedulingAlgo : %d\n", scheduler->schedulingAlgo);

        count++;
    }

    for (int i = 0; i < count; ++i)
    {
        OpenFile* exec;
        NachOSThread * newThread;
        exec = fileSystem->Open(executable[i]);
        ASSERT(exec!= NULL);
        newThread = new NachOSThread(executable[i], priority[i]);
        // printf("new thread created with basePriority %d\n", newThread->basePriority);
        newThread->space = new ProcessAddressSpace(exec);
        newThread->space->InitUserModeCPURegisters();

        newThread->SaveUserState();// newThread->space->RestoreContextOnSwitch(); this gives error :(

        newThread->CreateThreadStack(BatchStarter, 0);

        newThread->Schedule();
                // printf("+++++++++ +++++++++++++++++++++++++++++++++++++++++++++\n");

        delete exec;
    }

    int q = 100;
    if(scheduler->schedulingAlgo == 3 || scheduler->schedulingAlgo == 7){
        q = (int)(118.16/4);
    }
    else if(scheduler->schedulingAlgo == 4 || scheduler->schedulingAlgo == 8){
        q = (int)(118.16/2);
    }
    else if(scheduler->schedulingAlgo == 5 || scheduler->schedulingAlgo == 9){
        q = (int)(3*118.16/4);
    }
    else if(scheduler->schedulingAlgo == 6 || scheduler->schedulingAlgo == 10){
        q = 20;
    }

    // scheduler->quantum = q;
    stats->TimerTicks = q;          //
    //stats->TimerTicks = 100;        //for part 4

    currentThread->Exit(false, 0);
    // printf("++++++\n");
// 
}

void
LaunchUserProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);
    ProcessAddressSpace *space;

    if (executable == NULL) {
	printf("Unable to open file %s\n", filename);
	return;
    }
    space = new ProcessAddressSpace(executable);    
    currentThread->space = space;

    delete executable;			// close file

    space->InitUserModeCPURegisters();		// set the initial register values
    space->RestoreContextOnSwitch();		// load page table register

    machine->Run();			// jump to the user progam
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
