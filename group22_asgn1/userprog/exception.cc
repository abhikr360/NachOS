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
#include "machine.h"
#include "utility.h"
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
extern void LaunchUserProcess(char*);
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
func (int a){
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
	threadToBeDestroyed = NULL;
    }
    
#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {		// if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
	currentThread->space->RestoreContextOnSwitch();
	machine->Run();
    }
#endif
//	currentThread->DeleteUtil();
//	scheduler->ScheduleThread(currentThread);
//	machine->Run();

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

    if ((which == SyscallException) && (type == SysCall_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();
    }
    else if ((which == SyscallException) && (type == SysCall_PrintInt)) {
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
    else if ((which == SyscallException) && (type == SysCall_PrintChar)) {
	writeDone->P() ;

        console->PutChar(machine->ReadRegister(4));   // echo it!
       // Advance program counters.
		//printf("MACHINE READ %d\n" , machine->ReadRegister(PCReg));
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));

       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);


    }
    else if ((which == SyscallException) && (type == SysCall_PrintString)) {
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
    else if ((which == SyscallException) && (type == SysCall_PrintIntHex)) {
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
       // Advance program counters.
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
    } 
	else if((which == SyscallException) && (type == SysCall_GetReg)){
	unsigned registernumber = (unsigned)machine->ReadRegister(4);
      	machine->WriteRegister(2, machine->ReadRegister(registernumber));
      	//printf("SysCall_GetReg done :P\n");		
	 machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
	
	}
	else if((which == SyscallException) && (type == SysCall_GetPA)){
		
		vaddr = (unsigned)machine->ReadRegister(4);
		unsigned pageNumber = (unsigned)vaddr/PageSize;
		//printf("PageNumber : %u", pageNumber);
		unsigned offset = (unsigned) vaddr%PageSize;
		//printf("offset : %u", offset);
		if(pageNumber >= machine->pageTableSize ) machine->WriteRegister(2,-1);
		TranslationEntry * KernelPageTable = machine->KernelPageTable;
		if(!KernelPageTable[pageNumber].valid)  machine->WriteRegister(2,-1);
		TranslationEntry* entry =&KernelPageTable[pageNumber];
		unsigned int pageFrame = entry->physicalPage;
		machine->WriteRegister(2, pageFrame * PageSize + offset);		
		
	 machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

	}
	else if((which == SyscallException) && (type == SysCall_GetPID)){
		machine->WriteRegister(2,currentThread->getPid());
		//printf("Pid returned :)\n");
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
	}
	else if((which == SyscallException) && (type == SysCall_GetPPID)){
       machine->WriteRegister(2,currentThread->getPpid());
	//printf("Papa ka pid returned\n");
       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
	}	
	else if((which == SyscallException) && (type == SysCall_Time))
	{
		machine->WriteRegister(2,stats->totalTicks);
		//printf("%d Ticks returned\n", stats->totalTicks);
	       machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
       		machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
       	machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

	}
	
	else if((which == SyscallException) && (type == SysCall_Sleep))
        {
		long stime = machine->ReadRegister(4);
		if(stime==0){
			currentThread->YieldCPU();
		
		}else{
			currentThread->GoToSleep(stats->totalTicks+stime);		

		}
         machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
         machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

        }
	else if((which == SyscallException) && (type == SysCall_Yield))
        {
               currentThread->YieldCPU();
                //printf("Yield CPU called\n");
               machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
                machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

        }
	else if((which == SyscallException) && (type == SysCall_NumInstr))
        {
              machine->WriteRegister(2,machine->numInstr); 
                //printf("number of Instruction till now : %ld\n",machine->numInstr);
               machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
                machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

        }
	else if((which == SyscallException) && (type == SysCall_Exec))
	{
		vaddr = machine->ReadRegister(4);
		 //debug 001		
		 char filename[100];
		 int i=0;
		 machine->ReadMem(vaddr,1,&memval);
		 while(*(char*)&memval !='\0'){
		 	filename[i]=(*(char*)&memval);
		 	i++;
		 	vaddr++;
		 	machine->ReadMem(vaddr,1,&memval);
		 }
		 
		 filename[i]='\0';
		 LaunchUserProcess(filename);
		 
		 
		
	}
	else if((which == SyscallException) && (type == SysCall_Exit))
        {
        	int exitcode = machine->ReadRegister(4);
			threadToBeDestroyed = currentThread;
			//printf("pid : %d  called exit code %d\n",currentThread->getPid(),exitcode);
			interrupt->SetLevel(IntOff);
        	//currentThread->PutThreadToSleep();
       // (void) interrupt->SetLevel(oldLevel);
			//currentThread->PutThreadToSleep();
      if(currentThread->getPpid()>0){
 	   scheduler->MoveThreadToReadyQueue(globalThread[currentThread->getPpid()]);
        globalThread[currentThread->getPpid()]->exitChild(currentThread->getPid(),exitcode);
      }

			
			NachOSThread * nextThread ;//= new NachOSThread();
			
			nextThread = scheduler -> SelectNextReadyThread();
			while(nextThread == NULL){
				//halt
				if(WaitingList->IsEmpty()){
					interrupt->Halt();					
				}
				else {
					interrupt->Idle();
					
				}
				nextThread = scheduler -> SelectNextReadyThread();
				
//				printf("pid : %d  called exit code %d\n",currentThread->getPid(),exitcode);

			}
		
			//printf("pid : %d  called exit code %d\n",currentThread->getPid(),exitcode);
				//currentThread->PutThreadToSleep();
				scheduler->ScheduleThread(nextThread);
			
			
        }
        
        else if((which == SyscallException) && (type == SysCall_Fork))
        {
              
		DEBUG('a', "DOG Entered Fork.\n");
               machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
               machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
               machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);

	           NachOSThread * newThread = new NachOSThread("forked");
               //newThread->space = new ProcessAddressSpace(newThread->getPid(), currentThread->space);
               newThread->SaveUserState();
	           //newThread->CreateThreadStack(func,0);
	           newThread->setReturnToZero();
		DEBUG('a', "DOG PID %d.\n", newThread->getPid());
//printf("child PID [exception.cc]%d \n",newThread->getPid());
	           

			machine->WriteRegister(2, newThread->getPid());
             currentThread->addChild(newThread->getPid());
               newThread->space = new ProcessAddressSpace(newThread->getPid(), currentThread->space);
             newThread->ThreadFork(func, 0);
           //  printf("current PC forked : %d\n",machine->ReadRegister(PCReg));
             //currentThread->addChild(newThread->getPid());
			//machine->WriteRegister(2, newThread->getPid());
			//printf("child PID [exception.cc]%d \n",machine->ReadRegister(2));

	           //scheduler->MoveThreadToReadyQueue(newThread);
		       //printf("which = %d\n",which);

 				/*machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
               machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
               machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);*/
				
        }

	else if((which == SyscallException) && (type == SysCall_Join))
	{

		int waitchild = machine->ReadRegister(4);
    //printf("Wait Child : %d\n", waitchild);
		int temp = currentThread->isChild(waitchild);
		if(temp!= -1){
		  machine->WriteRegister(2, currentThread->joinWithChild(waitchild, temp ));
    }else{
		  machine->WriteRegister(2, -1);
	 }
		
		machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
    machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
    machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg)+4);
				
	}

	else {
//	printf("%d".which);
		
	currentThread->Print();
	//printf(" current PC : %d\n",machine->ReadRegister(PCReg));
	printf(" 	No syscall Detected Unexpected user mode exception %d %d\n", which, type);

	ASSERT(FALSE);
    }
}
