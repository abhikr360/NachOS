// thread.cc 
//	Routines to manage threads.  There are four main operations:
//
//	ThreadFork -- create a thread to run a procedure concurrently
//		with the caller (this is done in two steps -- first
//		allocate the NachOSThread object, then call ThreadFork on it)
//	FinishThread -- called when the forked procedure finishes, to clean up
//	YieldCPU -- relinquish control over the CPU to another ready thread
//	PutThreadToSleep -- relinquish control over the CPU, but thread is now blocked.
//		In other words, it will not run again, until explicitly 
//		put back on the ready queue.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "thread.h"
#include "switch.h"
#include "synch.h"
#include "system.h"
#include "string.h"

#define STACK_FENCEPOST 0xdeadbeef	// this is put at the top of the
					// execution stack, for detecting 
					// stack overflows

//----------------------------------------------------------------------
// NachOSThread::NachOSThread
// 	Initialize a thread control block, so that we can then call
//	NachOSThread::ThreadFork.
//
//	"threadName" is an arbitrary string, useful for debugging.
//----------------------------------------------------------------------
int NachOSThread::totalNumberOfThreads=0;


NachOSThread::NachOSThread(char* threadName)
{
    
    totalNumberOfThreads++;
    int i;
    name = threadName;
    stackTop = NULL;
    stack = NULL;
    status = JUST_CREATED;
#ifdef USER_PROGRAM
    space = NULL;
    stateRestored = true;
#endif

    startTime=0;
    finishTime=0;
    totalTime=0;

    startCpuBurst=0;
    prevCpuBurst=0;
    predictedCpuBurst=256.0;
    totalCpuBurst = 0;
    cpuUsage=0;

    waitStart=0;
    waitEnd=0;
    waitTotal=0;

    blockStart=0;
    blockEnd=0;
    blockTotal=0;

    hasQuantaExpired=0;

    threadArray[thread_index] = this;
    pid = thread_index;
    thread_index++;
    ASSERT(thread_index < MAX_THREAD_COUNT);
    if (currentThread != NULL) {
       ppid = currentThread->GetPID();
       currentThread->RegisterNewChild (pid);
    }
    else ppid = -1;

    childcount = 0;
    waitchild_id = -1;

    for (i=0; i<MAX_CHILD_COUNT; i++) exitedChild[i] = false;

    instructionCount = 0;

}

NachOSThread::NachOSThread(char* threadName, int pty)
{
    totalNumberOfThreads++;
    int i;
    basePriority = 50 + pty;
    priority = 50 + pty;

    name = threadName;
    stackTop = NULL;
    stack = NULL;
    status = JUST_CREATED;
#ifdef USER_PROGRAM
    space = NULL;
    stateRestored = true;
#endif

    startTime=0;
    finishTime=0;
    totalTime=0;

    startCpuBurst=0;
    prevCpuBurst=0;
    predictedCpuBurst=256.0;
    totalCpuBurst = 0;
    cpuUsage=0;

    waitStart=0;
    waitEnd=0;
    waitTotal=0;

    blockStart=0;
    blockEnd=0;
    blockTotal=0;

    hasQuantaExpired=0;

    threadArray[thread_index] = this;
    pid = thread_index;
    thread_index++;
    ASSERT(thread_index < MAX_THREAD_COUNT);
    if (currentThread != NULL) {
       ppid = currentThread->GetPID();
       currentThread->RegisterNewChild (pid);
    }
    else ppid = -1;

    childcount = 0;
    waitchild_id = -1;

    for (i=0; i<MAX_CHILD_COUNT; i++) exitedChild[i] = false;

    instructionCount = 0;
}


//----------------------------------------------------------------------
// NachOSThread::~NachOSThread
// 	De-allocate a thread.
//
// 	NOTE: the current thread *cannot* delete itself directly,
//	since it is still running on the stack that we need to delete.
//
//      NOTE: if this is the main thread, we can't delete the stack
//      because we didn't allocate it -- we got it automatically
//      as part of starting up Nachos.
//----------------------------------------------------------------------

NachOSThread::~NachOSThread()
{
  totalNumberOfThreads--;
  // printf("(%d)\n", totalNumberOfThreads);
    DEBUG('t', "Deleting thread \"%s\" with pid %d\n", name, pid);

    // printf("destructor : %s\n", currentThread->getName());
    ASSERT(this != currentThread);
    if (stack != NULL)
	DeallocBoundedArray((char *) stack, StackSize * sizeof(int));
}

//----------------------------------------------------------------------
// NachOSThread::ThreadFork
// 	Invoke (*func)(arg), allowing caller and callee to execute 
//	concurrently.
//
//	NOTE: although our definition allows only a single integer argument
//	to be passed to the procedure, it is possible to pass multiple
//	arguments by making them fields of a structure, and passing a pointer
//	to the structure as "arg".
//
// 	Implemented as the following steps:
//		1. Allocate a stack
//		2. Initialize the stack so that a call to SWITCH will
//		cause it to run the procedure
//		3. Put the thread on the ready queue
// 	
//	"func" is the procedure to run concurrently.
//	"arg" is a single argument to be passed to the procedure.
//----------------------------------------------------------------------

void 
NachOSThread::ThreadFork(VoidFunctionPtr func, int arg)
{
    DEBUG('t', "Forking thread \"%s\" with pid %d with func = 0x%x, arg = %d\n",
	  name, pid, (int) func, arg);
    
    CreateThreadStack(func, arg);

    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    scheduler->MoveThreadToReadyQueue(this);	// MoveThreadToReadyQueue assumes that interrupts 
					// are disabled!
    (void) interrupt->SetLevel(oldLevel);
}    

//----------------------------------------------------------------------
// NachOSThread::CheckOverflow
// 	Check a thread's stack to see if it has overrun the space
//	that has been allocated for it.  If we had a smarter compiler,
//	we wouldn't need to worry about this, but we don't.
//
// 	NOTE: Nachos will not catch all stack overflow conditions.
//	In other words, your program may still crash because of an overflow.
//
// 	If you get bizarre results (such as seg faults where there is no code)
// 	then you *may* need to increase the stack size.  You can avoid stack
// 	overflows by not putting large data structures on the stack.
// 	Don't do this: void foo() { int bigArray[10000]; ... }
//----------------------------------------------------------------------

void
NachOSThread::CheckOverflow()
{
    if (stack != NULL)
#ifdef HOST_SNAKE			// Stacks grow upward on the Snakes
	ASSERT(stack[StackSize - 1] == STACK_FENCEPOST);
#else
	ASSERT(*stack == STACK_FENCEPOST);
#endif
}

//----------------------------------------------------------------------
// NachOSThread::FinishThread
// 	Called by ThreadRoot when a thread is done executing the 
//	forked procedure.
//
// 	NOTE: we don't immediately de-allocate the thread data structure 
//	or the execution stack, because we're still running in the thread 
//	and we're still on the stack!  Instead, we set "threadToBeDestroyed", 
//	so that ProcessScheduler::Schedule() will call the destructor, once we're
//	running in the context of a different thread.
//
// 	NOTE: we disable interrupts, so that we don't get a time slice 
//	between setting threadToBeDestroyed, and going to sleep.
//----------------------------------------------------------------------

//
void
NachOSThread::FinishThread ()
{
    (void) interrupt->SetLevel(IntOff);		
    ASSERT(this == currentThread);
    
    DEBUG('t', "Finishing thread \"%s\" with pid %d\n", getName(), pid);
    
    threadToBeDestroyed = currentThread;
    PutThreadToSleep();					// invokes SWITCH
    // not reached
}

//----------------------------------------------------------------------
// NachOSThread::SetChildExitCode
//      Called by an exiting thread on parent's thread object.
//----------------------------------------------------------------------

void
NachOSThread::SetChildExitCode (int childpid, int ecode)
{
   unsigned i;

   // Find out which child
   for (i=0; i<childcount; i++) {
      if (childpid == childpidArray[i]) break;
   }
   printf("Child Count : %d, i : %d\n", childcount, i);
   ASSERT(i < childcount);//Failing because there is no parent
   childexitcode[i] = ecode;
   exitedChild[i] = true;

   if (waitchild_id == (int)i) {
      waitchild_id = -1;
      // I will wake myself up
      IntStatus oldLevel = interrupt->SetLevel(IntOff);
      scheduler->MoveThreadToReadyQueue(this);
      (void) interrupt->SetLevel(oldLevel);
   }
}

//----------------------------------------------------------------------
// NachOSThread::Exit
//      Called by ExceptionHandler when a thread calls Exit.
//      The argument specifies if all threads have called Exit, in which
//      case, the simulation should be terminated.
//----------------------------------------------------------------------

void
NachOSThread::Exit (bool terminateSim, int exitcode)
{
    (void) interrupt->SetLevel(IntOff);
    ASSERT(this == currentThread);

    DEBUG('t', "Finishing thread \"%s\" with pid %d\n", getName(), pid);

    statThreadUpdate('e');

    threadToBeDestroyed = currentThread;
    NachOSThread *nextThread;


    //UNIX

    if(currentThread->prevCpuBurst > 0 && scheduler->schedulingAlgo >= 7 && scheduler->schedulingAlgo <= 10){
        currentThread->cpuUsage = currentThread->cpuUsage + currentThread->prevCpuBurst;

        for (int i = 0; i < MAX_THREAD_COUNT; ++i)
        {
            if(threadArray[i] != NULL){
                threadArray[i]->cpuUsage = (threadArray[i]->cpuUsage)/2;
                threadArray[i]->priority = threadArray[i]->basePriority + (threadArray[i]->cpuUsage)/2;
            }
        }
    }

    status = BLOCKED;

    currentThread->blockStart = stats->totalTicks;


    for (int i = 0; i < childcount; ++i)
    {
      threadArray[childpidArray[i]]->setPPID(-1);
    }
    threadArray[currentThread->GetPID()]=NULL;
    exitThreadArray[currentThread->GetPID()]=true;

    if(totalNumberOfThreads == 1 ){
      terminateSim = true;
    }

    // Set exit code in parent's structure provided the parent hasn't exited
    if (ppid != -1) {
       ASSERT(threadArray[ppid] != NULL);
       if (!exitThreadArray[ppid]) {
          threadArray[ppid]->SetChildExitCode (pid, exitcode);
       }
    }

    //Stat Update
    
    if(currentThread->GetPID()){
        int finishTime = stats->totalTicks;
    DEBUG('t', "updating stats \"%s\" with pid %d and finish time %d\n", getName(), pid,finishTime);

        stats->totalProcessEndTime = stats->totalProcessEndTime + finishTime;

        stats->squareSumProcessEndTime = stats->squareSumProcessEndTime + (long long)(finishTime)*(long long)finishTime;
        
        stats->maxProcessEndTime = (stats->maxProcessEndTime > finishTime) ?  stats->maxProcessEndTime: finishTime ;
        stats->minProcessEndTime = (stats->minProcessEndTime < finishTime) ?  stats->minProcessEndTime: finishTime ;
        stats->countOverallBurst++;
        stats->overallCpuBurst = stats->overallCpuBurst + currentThread->prevCpuBurst;
        stats->maxCpuBurst = (currentThread->prevCpuBurst > stats->maxCpuBurst) ? currentThread->prevCpuBurst : stats->maxCpuBurst;
        stats->minCpuBurst = (currentThread->prevCpuBurst < stats->minCpuBurst) ? currentThread->prevCpuBurst : stats->minCpuBurst;
        stats->numThread++;
        stats->totalTimeInThread = stats->totalTimeInThread + currentThread->totalTime;
        stats->longestThread = (stats->longestThread > currentThread->totalTime) ? stats->longestThread: currentThread->totalTime ;
        stats->shortestThread = (stats->shortestThread < currentThread->totalTime) ? stats->shortestThread: currentThread->totalTime ;
        stats->overallWait = stats->overallWait+currentThread->waitTotal;
    }

    
    while ((nextThread = scheduler->SelectNextReadyThread()) == NULL) {
        //printf("Inside Exit ka loop\n");
        if (terminateSim) {
           DEBUG('i', "Machine idle.  No interrupts to do.\n");
           printf("\nNo threads ready or runnable, and no pending interrupts.\n");
           printf("Assuming all programs completed.\n");
           interrupt->Halt();
        }
        else interrupt->Idle();      // no one to run, wait for an interrupt
    }

    currentThread->blockEnd = stats->totalTicks;
    currentThread->blockTotal = currentThread->blockTotal + (currentThread->blockEnd - currentThread->blockStart);

    //6printf("TerminateSim = %d\n", (terminateSim==true));
    scheduler->ScheduleThread(nextThread); // returns when we've been signalled
}

//----------------------------------------------------------------------
// NachOSThread::YieldCPU
// 	Relinquish the CPU if any other thread is ready to run.
//	If so, put the thread on the end of the ready list, so that
//	it will eventually be re-scheduled.
//
//	NOTE: returns immediately if no other thread on the ready queue.
//	Otherwise returns when the thread eventually works its way
//	to the front of the ready list and gets re-scheduled.
//
//	NOTE: we disable interrupts, so that looking at the thread
//	on the front of the ready list, and switching to it, can be done
//	atomically.  On return, we re-set the interrupt level to its
//	original state, in case we are called with interrupts disabled. 
//
// 	Similar to NachOSThread::PutThreadToSleep(), but a little different.
//----------------------------------------------------------------------

void
NachOSThread::YieldCPU ()
{
    NachOSThread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    
    ASSERT(this == currentThread);
    // printf("Yielding!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    DEBUG('t', "Yielding thread \"%s\" with pid %d\n", getName(), pid);

    //printf("================================================%d\n", currentThread->prevCpuBurst);
    //printf("================================================%d\n", currentThread->startCpuBurst);
    //printf("==========================+======================%d\n", currentThread->prevCpuBurst);

    statThreadUpdate('y');
    
    
    //UNIX

    if(currentThread->prevCpuBurst > 0 && scheduler->schedulingAlgo >= 7 && scheduler->schedulingAlgo <= 10){
        currentThread->cpuUsage = currentThread->cpuUsage + currentThread->prevCpuBurst;

        for (int i = 0; i < MAX_THREAD_COUNT; ++i)
        {
            if(threadArray[i] != NULL){
                threadArray[i]->cpuUsage = (threadArray[i]->cpuUsage)/2;
                threadArray[i]->priority = threadArray[i]->basePriority + (threadArray[i]->cpuUsage)/2;
            }
        }

            

    }
   
    if(hasQuantaExpired==1){//This thread should also be a part of input to Scheduling algo if this is a timeryield.
        scheduler->MoveThreadToReadyQueue(this);
    }

    nextThread = scheduler->SelectNextReadyThread();

// printf("Yielding %s---\n",currentThread->getName());

    if (nextThread != NULL)
    {

        if(scheduler->schedulingAlgo>=7 && nextThread->priority > currentThread->priority)
        {
            scheduler->MoveThreadToReadyQueue(nextThread);
            scheduler->ScheduleThread(currentThread);

        }
        else
        {
            if(hasQuantaExpired == 0)
                scheduler->MoveThreadToReadyQueue(this);
           else
           {
                hasQuantaExpired = 0;
           }
           scheduler->ScheduleThread(nextThread);
        }
	  
    }
    else
    {
        currentThread->startCpuBurst = stats->totalTicks;
    }


    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// NachOSThread::PutThreadToSleep
// 	Relinquish the CPU, because the current thread is blocked
//	waiting on a synchronization variable (Semaphore, Lock, or Condition).
//	Eventually, some thread will wake this thread up, and put it
//	back on the ready queue, so that it can be re-scheduled.
//
//	NOTE: if there are no threads on the ready queue, that means
//	we have no thread to run.  "Interrupt::Idle" is called
//	to signify that we should idle the CPU until the next I/O interrupt
//	occurs (the only thing that could cause a thread to become
//	ready to run).
//
//	NOTE: we assume interrupts are already disabled, because it
//	is called from the synchronization routines which must
//	disable interrupts for atomicity.   We need interrupts off 
//	so that there can't be a time slice between pulling the first thread
//	off the ready list, and switching to it.
//----------------------------------------------------------------------
void
NachOSThread::PutThreadToSleep ()
{
    NachOSThread *nextThread;
    
    ASSERT(this == currentThread);
    ASSERT(interrupt->getLevel() == IntOff);

    DEBUG('t', "Sleeping thread \"%s\" with pid %d\n", getName(), pid);
    

    statThreadUpdate('s');

    //UNIX

    if(currentThread->prevCpuBurst > 0 && scheduler->schedulingAlgo >= 7 && scheduler->schedulingAlgo <= 10){
        currentThread->cpuUsage = currentThread->cpuUsage + currentThread->prevCpuBurst;

        for (int i = 0; i < MAX_THREAD_COUNT; ++i)
        {
            if(threadArray[i] != NULL){
                threadArray[i]->cpuUsage = (threadArray[i]->cpuUsage)/2;
                threadArray[i]->priority = threadArray[i]->basePriority + (threadArray[i]->cpuUsage)/2;
            }
        }
    }




    currentThread->blockStart = stats->totalTicks;
    status = BLOCKED;

    currentThread->blockStart = stats->totalTicks;


    while ((nextThread = scheduler->SelectNextReadyThread()) == NULL){
        //printf("'INside put thread to sleep ka loop'\n");
	interrupt->Idle();	// no one to run, wait for an interrupt
    }

    currentThread->blockEnd = stats->totalTicks;
    currentThread->blockTotal = currentThread->blockTotal + currentThread->blockEnd - currentThread->blockStart;
        
    scheduler->ScheduleThread(nextThread); // returns when we've been signalled
}

//----------------------------------------------------------------------
// ThreadFinish, InterruptEnable, ThreadPrint
//	Dummy functions because C++ does not allow a pointer to a member
//	function.  So in order to do this, we create a dummy C function
//	(which we can pass a pointer to), that then simply calls the 
//	member function.
//----------------------------------------------------------------------

static void ThreadFinish()    { currentThread->FinishThread(); }
static void InterruptEnable() { interrupt->Enable(); }
void ThreadPrint(int arg){ NachOSThread *t = (NachOSThread *)arg; t->Print(); }

//----------------------------------------------------------------------
// NachOSThread::CreateThreadStack
//	Allocate and initialize an execution stack.  The stack is
//	initialized with an initial stack frame for ThreadRoot, which:
//		enables interrupts
//		calls (*func)(arg)
//		calls NachOSThread::FinishThread
//
//	"func" is the procedure to be forked
//	"arg" is the parameter to be passed to the procedure
//----------------------------------------------------------------------

void
NachOSThread::CreateThreadStack (VoidFunctionPtr func, int arg)
{
    stack = (int *) AllocBoundedArray(StackSize * sizeof(int));

#ifdef HOST_SNAKE
    // HP stack works from low addresses to high addresses
    stackTop = stack + 16;	// HP requires 64-byte frame marker
    stack[StackSize - 1] = STACK_FENCEPOST;
#else
    // i386 & MIPS & SPARC stack works from high addresses to low addresses
#ifdef HOST_SPARC
    // SPARC stack must contains at least 1 activation record to start with.
    stackTop = stack + StackSize - 96;
#else  // HOST_MIPS  || HOST_i386
    stackTop = stack + StackSize - 4;	// -4 to be on the safe side!
#ifdef HOST_i386
    // the 80386 passes the return address on the stack.  In order for
    // SWITCH() to go to ThreadRoot when we switch to this thread, the
    // return addres used in SWITCH() must be the starting address of
    // ThreadRoot.
    *(--stackTop) = (int)_ThreadRoot;
#endif
#endif  // HOST_SPARC
    *stack = STACK_FENCEPOST;
#endif  // HOST_SNAKE
    
    machineState[PCState] = (int) _ThreadRoot;
    machineState[StartupPCState] = (int) InterruptEnable;
    machineState[InitialPCState] = (int) func;
    machineState[InitialArgState] = arg;
    machineState[WhenDonePCState] = (int) ThreadFinish;
}

#ifdef USER_PROGRAM
#include "machine.h"

//----------------------------------------------------------------------
// NachOSThread::SaveUserState
//	Save the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers -- 
//	one for its state while executing user code, one for its state 
//	while executing kernel code.  This routine saves the former.
//----------------------------------------------------------------------

void
NachOSThread::SaveUserState()
{
    if (stateRestored) {
       for (int i = 0; i < NumTotalRegs; i++)
	   userRegisters[i] = machine->ReadRegister(i);
       stateRestored = false;
    }
}

//----------------------------------------------------------------------
// NachOSThread::RestoreUserState
//	Restore the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers -- 
//	one for its state while executing user code, one for its state 
//	while executing kernel code.  This routine restores the former.
//----------------------------------------------------------------------

void
NachOSThread::RestoreUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, userRegisters[i]);
    stateRestored = true;
}
#endif

//----------------------------------------------------------------------
// NachOSThread::CheckIfChild
//      Checks if the passed pid belongs to a child of mine.
//      Returns child id if all is fine; otherwise returns -1.
//----------------------------------------------------------------------

int
NachOSThread::CheckIfChild (int childpid)
{
   unsigned i;

   // Find out which child
   for (i=0; i<childcount; i++) {
      if (childpid == childpidArray[i]) break;
   }

   if (i == childcount) return -1;
   return i;
}

//----------------------------------------------------------------------
// NachOSThread::JoinWithChild
//      Called by a thread as a result of SysCall_Join.
//      Returns the exit code of the child being joined with.
//----------------------------------------------------------------------

int
NachOSThread::JoinWithChild (int whichchild)
{
   // Has the child exited?
   if (!exitedChild[whichchild]) {
      // Put myself to sleep
      waitchild_id = whichchild;
      IntStatus oldLevel = interrupt->SetLevel(IntOff);
      printf("[pid %d] Before sleep in JoinWithChild.\n", pid);
      PutThreadToSleep();
      printf("[pid %d] After sleep in JoinWithChild.\n", pid);
      (void) interrupt->SetLevel(oldLevel);
   }
   return childexitcode[whichchild];
}

#ifdef USER_PROGRAM
//----------------------------------------------------------------------
// NachOSThread::ResetReturnValue
//      Sets the syscall return value to zero. Used to set the return
//      value of SysCall_Fork in the created child.
//----------------------------------------------------------------------

void
NachOSThread::ResetReturnValue ()
{
   userRegisters[2] = 0;
}
#endif

//----------------------------------------------------------------------
// NachOSThread::Schedule
//      Enqueues the thread in the ready queue.
//----------------------------------------------------------------------

void
NachOSThread::Schedule()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    scheduler->MoveThreadToReadyQueue(this);        // MoveThreadToReadyQueue assumes that interrupts
                                        // are disabled!

    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// NachOSThread::Startup
//      Part of the scheduling code needed to cleanly start a forked child.
//----------------------------------------------------------------------

void
NachOSThread::Startup()
{
   scheduler->Tail();
}

//----------------------------------------------------------------------
// NachOSThread::SortedInsertInWaitQueue
//      Called by SysCall_Sleep before putting the caller thread to sleep
//----------------------------------------------------------------------

void
NachOSThread::SortedInsertInWaitQueue (unsigned when)
{
   TimeSortedWaitQueue *ptr, *prev, *temp;

   if (sleepQueueHead == NULL) {
      sleepQueueHead = new TimeSortedWaitQueue (this, when);
      ASSERT(sleepQueueHead != NULL);
   }
   else {
      ptr = sleepQueueHead;
      prev = NULL;
      while ((ptr != NULL) && (ptr->GetWhen() <= when)) {
         prev = ptr;
         ptr = ptr->GetNext();
      }
      if (ptr == NULL) {  // Insert at tail
         ptr = new TimeSortedWaitQueue (this, when);
         ASSERT(ptr != NULL);
         ASSERT(prev->GetNext() == NULL);
         prev->SetNext(ptr);
      }
      else if (prev == NULL) {  // Insert at head
         ptr = new TimeSortedWaitQueue (this, when);
         ASSERT(ptr != NULL);
         ptr->SetNext(sleepQueueHead);
         sleepQueueHead = ptr;
      }
      else {
         temp = new TimeSortedWaitQueue (this, when);
         ASSERT(temp != NULL);
         temp->SetNext(ptr);
         prev->SetNext(temp);
      }
   }

   IntStatus oldLevel = interrupt->SetLevel(IntOff);
   //printf("[pid %d] Going to sleep at %d.\n", pid, stats->totalTicks);
   PutThreadToSleep();
   //printf("[pid %d] Returned from sleep at %d.\n", pid, stats->totalTicks);
   (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// NachOSThread::IncInstructionCount
//      Called by Machine::Run to update instruction count
//----------------------------------------------------------------------

void
NachOSThread::IncInstructionCount (void)
{
   instructionCount++;
}

//----------------------------------------------------------------------
// NachOSThread::GetInstructionCount
//      Called by SysCall_NumInstr
//----------------------------------------------------------------------

unsigned
NachOSThread::GetInstructionCount (void)
{
   return instructionCount;
}

void
NachOSThread::statThreadUpdate(char a){
    if(a == 'y')//updating stat for YieldCpu
    {
        currentThread->prevCpuBurst = stats->totalTicks - currentThread->startCpuBurst;
        currentThread->totalCpuBurst = currentThread->totalCpuBurst + currentThread->prevCpuBurst;

        if(currentThread->GetPID()){
            stats->countOverallBurst++;
            stats->overallCpuBurst = stats->overallCpuBurst + currentThread->prevCpuBurst;
            //printf("==========================+++======================%d\n", stats->overallCpuBurst);
            stats->maxCpuBurst = (currentThread->prevCpuBurst > stats->maxCpuBurst) ? currentThread->prevCpuBurst : stats->maxCpuBurst;
            stats->minCpuBurst = (currentThread->prevCpuBurst < stats->minCpuBurst) ? currentThread->prevCpuBurst : stats->minCpuBurst;
        }
    }

    else if(a == 'e')//updating stat for Exit
    {
        currentThread->prevCpuBurst = stats->totalTicks - currentThread->startCpuBurst;
        currentThread->totalCpuBurst = currentThread->totalCpuBurst + currentThread->prevCpuBurst;

        float error = currentThread->prevCpuBurst - currentThread->predictedCpuBurst;
        error = error - 2*(error<0)*error;
        stats->predError = stats->predError+error;

    }
    else if(a == 's')//updating stat for PutThreadTosleep
    {
        currentThread->prevCpuBurst = stats->totalTicks - currentThread->startCpuBurst;
        currentThread->totalCpuBurst = currentThread->totalCpuBurst + currentThread->prevCpuBurst;

        if(currentThread->GetPID()){
            stats->countOverallBurst++;
            stats->overallCpuBurst = stats->overallCpuBurst + currentThread->prevCpuBurst;
            stats->maxCpuBurst = (currentThread->prevCpuBurst > stats->maxCpuBurst) ? currentThread->prevCpuBurst : stats->maxCpuBurst;
            stats->minCpuBurst = (currentThread->prevCpuBurst < stats->minCpuBurst) ? currentThread->prevCpuBurst : stats->minCpuBurst;
        }

    }
}
