							CS330 - NACHOS Assignment 
========================================================
								By - Group No. 22
========================================================

#SysCall_GetReg:
----------------
This was a simple system call, we just read the value of the register using the function ReadRegister() and then returned the value in register 2 by using the function WriteRegister().

#SysCall_GetPA:
----------------
We have taken inspiration from the method machine::Translate to get the physical address from the given Virtual Address and so we calculate the virtual pageNumber, offset and the physical page number of the physicall address from the given address. 

We have also handled all the three conditions where the SysCall should fail, & so we return -1 in those conditions.

#SysCall_GetPID:
-----------------
In this SysCall we simply created the method NachOSThread::getPid() which
 returns the value of the variable "int pid" defined in the NachOSThread Class.
 Files Modified:
 thread.cc
 thread.h

#SysCall_GetPPID:
-----------------
In this SysCall we created a method NachOSThread::getPpid() which
 returns the value of the variable "int ppid" defined in the NachOSThread Class.
Files Modified:
 thread.cc
 thread.h

#SysCall_NumInstr:
-----------------
Here we declared a variable numInstr in a few files as mentioned below and added this variable to the method Machine::OneInstruction. So whenever this method is called the value of numInstr increases by 1. And as we know that this method is called every time whenever nachOS has to execute any instruction. Hence the value stored in this variable gives us the total number of instructions executed from the beginning.

Files Modified:
machine.h <variable numInstr declared>
stats.h <variable numInstr declared>
mipssim.cc


#SysCall_Time:
-----------------
In this Syscall we simply return the value stored in the variable stats->totalTicks. This variable was defined by default in the file stats.h

#SysCall_Yield:
----------------
In this Syscall we simply call the function YieldCPU already defined in NachosThread class after saving context of process.


#SysCall_Sleep:
-----------------
For this Syscall we have created a method GoToSleep() in thread.cc.
If the argument passed in the Syscall is zero, then we call YieldCPU(), otherwise we call the function GoToSleep().

Functioning of GoToSleep():
	-here we add the current thread to a waiting queue(sorted with respect to time.)
	-then we disable the interrupt handler
	-this function calls the function NachOSThread::PutThreadToSleep.


#SysCall_Exec:
-----------------
We implemented it in the following manner:
1. We extracted the filename from the argument passed in the register.
2. Then we called the function LaunchUserProcess() with the filename passed as an argument to it.
	LaunchUserProcess() was defined by default in progtest.cc
	In this function, we create an instance of OpenFile with this filename unless the filename being null. After that we allocate a new address space for the process to be run with the given filename. Then we close the file and set the initial register values & load the page table register.


#SysCall_Exit:
-----------------
Here we first read the exitcode from the register and set the current thread as the thread to be destroyed.
Then we disable the interrupt.
Now we check if the PID of parent of the current process is greater than 0, then we call the funciton exitChild() with the exit code and current PID passed as arguments to this function.
After this we check for the next thread which is ready to be run, & while doing so, if we find the waiting list to be empty which means that no thread is sleeping then we call the Halt() function. If there is a process sleeping then we set the next thread in the ready queue to be scheduled for running just after this syscall returns.


#SysCall_Join:
---------------
We start by reading the PID of the thread which is to be joined with the current thread.
We now check if this thread if the child of the current thread. If so, then we join it with the current thread otherwise we return -1.
We call new method NachOSThread::joinWithChild() defined in thread.cc . It checks whether child has exited or not. If not parent goes to sleep. We do this by maintaing a boolean array for each process which stores whether child is running or not.


#SysCall_Fork:
-----------------
Here, first we advance the program counter then we declare a new thread variable which initializes the new thread to be created. 
Next we save the user state registers of the newly created thread.
Now we set the return value of the newly created thread to be zero.
For the current thread we set the return value of the syscall to be the PID of the newly created thread.
Also we update the child related fields in parent's memory.
Next we call the newly made constructor to allocate address space of child. Here we copy all the information from parent's memory to child's memory. 
Next, we call the ThreadFork function which creates stack for child and moves it to ready queue.


****************************THANK YOU******************


