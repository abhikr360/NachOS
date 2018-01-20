// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"

#define UserStackSize		1024 	// increase this as necessary!

class ProcessAddressSpace {
  public:
    static int end;
    void setNumVirtualPages(unsigned int num); 
    ProcessAddressSpace(OpenFile *executable);	// Create an address space,
   ProcessAddressSpace(int PID,ProcessAddressSpace* ppaddress);					// initializing it with the program
					// stored in the file "executable"
    ~ProcessAddressSpace();			// De-allocate an address space

    void InitUserModeCPURegisters();		// Initialize user-level CPU registers,
   unsigned int getNumVirtualPages();					// before jumping to user code
    TranslationEntry* getKernelPageTable();
    void SaveContextOnSwitch();			// Save/restore address space-specific
    void RestoreContextOnSwitch();		// info on a context switch 

  private:
    TranslationEntry *KernelPageTable;	// Assume linear page table translation
					// for now!
    unsigned int numVirtualPages;		// Number of pages in the virtual 
					// address space
};

#endif // ADDRSPACE_H
