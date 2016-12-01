// addrspace.cc
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include <stdlib.h>
#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// ProcessAddrSpace::ProcessAddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

ProcessAddrSpace::ProcessAddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;
    unsigned vpn, offset;
    TranslationEntry *entry;
    unsigned int pageFrame;

		//printf("(ProcessAddrSpace)\n");

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) &&
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPagesInVM = divRoundUp(size, PageSize);
    size = numPagesInVM * PageSize;

		printf("Num pages: %d size: %d numPagesAllocated =%d\n",numPagesInVM, size, numPagesAllocated);
		printf("[NOFFH] code== size: %d virtualAddr: %d inFileAddr: %d\n",noffH.code.size, noffH.code.virtualAddr, noffH.code.inFileAddr);
		printf("[NOFFH] initData== size: %d virtualAddr: %d inFileAddr: %d\n",noffH.initData.size, noffH.initData.virtualAddr, noffH.initData.inFileAddr);
		printf("[NOFFH] uninitData== size: %d virtualAddr: %d inFileAddr: %d\n",noffH.uninitData.size, noffH.uninitData.virtualAddr, noffH.uninitData.inFileAddr);
		printf("###########################################################\n");
    //ASSERT(numPagesInVM+numPagesAllocated <= NumPhysPages);		// check we're not trying
										// to run anything too big --
										// at least until we have
										// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
					numPagesInVM, size);
// first, set up the translation
    NachOSpageTable = new TranslationEntry[numPagesInVM];
		currentThread->SwapTable = new char[numPagesInVM*PageSize];
		bzero(currentThread->SwapTable, numPagesInVM*PageSize);

    for (i = 0; i < numPagesInVM; i++) {
			NachOSpageTable[i].virtualPage = i;
			NachOSpageTable[i].physicalPage = i+numPagesAllocated;
			NachOSpageTable[i].valid = FALSE;
			NachOSpageTable[i].use = FALSE;
			NachOSpageTable[i].dirty = FALSE;
			NachOSpageTable[i].readOnly = FALSE;  // if the code segment was entirely on
							// a separate page, we could set its
							// pages to be read-only
			NachOSpageTable[i].shared = FALSE;
			NachOSpageTable[i].swapped = FALSE;  // if the code segment was entirely on
    }
// zero out the entire address space, to zero the unitialized data segment
// and the stack segment
  //  bzero(&machine->mainMemory[numPagesAllocated*PageSize], size);

    //numPagesAllocated += numPagesInVM;

// then, copy in the code and data segments into memory
  /*  if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
			noffH.code.virtualAddr, noffH.code.size);
        vpn = noffH.code.virtualAddr/PageSize;
        offset = noffH.code.virtualAddr%PageSize;
        entry = &NachOSpageTable[vpn];
        pageFrame = entry->physicalPage;
        executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
			noffH.initData.virtualAddr, noffH.initData.size);
        vpn = noffH.initData.virtualAddr/PageSize;
        offset = noffH.initData.virtualAddr%PageSize;
        entry = &NachOSpageTable[vpn];
        pageFrame = entry->physicalPage;
        executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }
*/
}

//----------------------------------------------------------------------
// ProcessAddrSpace::ProcessAddrSpace (ProcessAddrSpace*) is called by a forked thread.
//      We need to duplicate the address space of the parent.
//----------------------------------------------------------------------

ProcessAddrSpace::ProcessAddrSpace(ProcessAddrSpace *parentSpace, int pid)
{
    numPagesInVM = parentSpace->GetNumPages();
    unsigned i, size = numPagesInVM * PageSize;
		unsigned unsharedPages = 0;
  //  ASSERT(numPagesInVM+numPagesAllocated <= NumPhysPages);                // check we're not trying
                                                                                // to run anything too big --
                                                                                // at least until we have
                                                                                // virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
                                        numPagesInVM, size);
    // first, set up the translation
    TranslationEntry* parentPageTable = parentSpace->GetPageTable();
    NachOSpageTable = new TranslationEntry[numPagesInVM];
    for (i = 0; i < numPagesInVM; i++) {
			if(!parentPageTable[i].shared){
				unsharedPages++;
        NachOSpageTable[i].virtualPage = i;
        //NachOSpageTable[i].physicalPage = i+numPagesAllocated;
				NachOSpageTable[i].valid = parentPageTable[i].valid;
				if(parentPageTable[i].valid)
				{
					NachOSpageTable[i].physicalPage = PagetoEvict(parentPageTable[i].physicalPage);  //Parent page shouldn't be swapped
					memcpy(&(machine->mainMemory[NachOSpageTable[i].physicalPage*PageSize]),
								 &(machine->mainMemory[parentPageTable[i].physicalPage*PageSize]), PageSize);

					machine->PhysMap[NachOSpageTable[i].physicalPage].processID = pid;
					machine->PhysMap[NachOSpageTable[i].physicalPage].virtPage = i;
					machine->PhysMap[NachOSpageTable[i].physicalPage].IsShared = FALSE;
					machine->PhysMap[NachOSpageTable[i].physicalPage].IsEmpty = FALSE;
					//numPagesAllocated++;
				}
				NachOSpageTable[i].use = parentPageTable[i].use;
        NachOSpageTable[i].dirty = parentPageTable[i].dirty;
        NachOSpageTable[i].readOnly = parentPageTable[i].readOnly;  	// if the code segment was entirely on
                                        			// a separate page, we could set its
                                        			// pages to be read-only
				NachOSpageTable[i].shared = parentPageTable[i].shared;
				NachOSpageTable[i].swapped = parentPageTable[i].swapped;
			}
			else{
				NachOSpageTable[i].virtualPage = i;
        NachOSpageTable[i].physicalPage = parentPageTable[i].physicalPage;
        NachOSpageTable[i].valid = parentPageTable[i].valid;
        NachOSpageTable[i].use = parentPageTable[i].use;
        NachOSpageTable[i].dirty = parentPageTable[i].dirty;
        NachOSpageTable[i].readOnly = parentPageTable[i].readOnly;  	// if the code segment was entirely on
                                        			// a separate page, we could set its
                                        			// pages to be read-only
				NachOSpageTable[i].shared = parentPageTable[i].shared;

				//Must be removed later
				ASSERT(parentPageTable[i].swapped == FALSE)

				NachOSpageTable[i].swapped = FALSE;

			}
    }

}

//----------------------------------------------------------------------
// ProcessAddrSpace::ProcessAddrSpace (ProcessAddrSpace*) is called by a forked thread.
//      We need to duplicate the address space of the parent.
//----------------------------------------------------------------------

ProcessAddrSpace::ProcessAddrSpace(ProcessAddrSpace *parentSpace, int sharedPages, unsigned *vaddr)
{
    numPagesInVM = parentSpace->GetNumPages() + sharedPages;
    unsigned i, size = numPagesInVM * PageSize;
  //  unsigned parentSize = parentSpace->GetNumPages()*PageSize;
    bool setSharedAddr = FALSE;
  //  ASSERT(numPagesInVM+numPagesAllocated <= NumPhysPages);                // check we're not trying
                                                                                // to run anything too big --
                                                                                // at least until we have
                                                                                // virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n",
                                        numPagesInVM, size);
    // first, set up the translation
    TranslationEntry* parentPageTable = parentSpace->GetPageTable();
    NachOSpageTable = new TranslationEntry[numPagesInVM];
    for (i = 0; i < numPagesInVM; i++) {
        NachOSpageTable[i].virtualPage = i;
        if(i < parentSpace->GetNumPages())
        {
						NachOSpageTable[i].physicalPage = parentPageTable[i].physicalPage;
            NachOSpageTable[i].valid = parentPageTable[i].valid;
            NachOSpageTable[i].use = parentPageTable[i].use;
            NachOSpageTable[i].dirty = parentPageTable[i].dirty;
            NachOSpageTable[i].readOnly = parentPageTable[i].readOnly;      // if the code segment was entirely on
            // a separate page, we could set its
            // pages to be read-only
            NachOSpageTable[i].shared = parentPageTable[i].shared;
						NachOSpageTable[i].swapped = FALSE;

        }
        else
        {   //Shared Pages
						NachOSpageTable[i].physicalPage = PagetoEvict(-1);  //Parent page shouldn't be swapped

						memset(&(machine->mainMemory[NachOSpageTable[i].physicalPage*PageSize]), 0, PageSize);

						machine->PhysMap[NachOSpageTable[i].physicalPage].processID = currentThread->GetPID();
						machine->PhysMap[NachOSpageTable[i].physicalPage].virtPage = i;
						machine->PhysMap[NachOSpageTable[i].physicalPage].IsShared = TRUE;
						machine->PhysMap[NachOSpageTable[i].physicalPage].IsEmpty = FALSE;

            NachOSpageTable[i].valid = TRUE;
            NachOSpageTable[i].use = FALSE;
            NachOSpageTable[i].dirty = FALSE;
            NachOSpageTable[i].readOnly = FALSE;
            NachOSpageTable[i].shared = TRUE;
						NachOSpageTable[i].swapped = FALSE;

            if(!setSharedAddr){
                *vaddr = i*PageSize;                                            // OFFSET is 0
                setSharedAddr = TRUE;
            }
        }
    }

    // Copy the contents
  //  unsigned startAddrParent = parentPageTable[0].physicalPage*PageSize;
    //unsigned startAddrChild = numPagesAllocated*PageSize;
    //for (i=0; i < parentSize; i++) {
      // machine->mainMemory[startAddrChild+i] = machine->mainMemory[startAddrParent+i];
  //  }

    //numPagesAllocated += numPagesInVM;
}

//----------------------------------------------------------------------
// ProcessAddrSpace::~ProcessAddrSpace
//  Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

ProcessAddrSpace::~ProcessAddrSpace()
{
   delete NachOSpageTable;
}


void
ProcessAddrSpace::FreePages(int pid)
{
  int i;
  //printf("FreePages %d\n",pid);
  for(i = 0; i < NumPhysPages; i++)
  {
    if(machine->PhysMap[i].processID == pid && machine->PhysMap[i].IsShared == FALSE)
    {
      machine->PhysMap[i].IsEmpty = TRUE;
      machine->PhysMap[i].processID = -1;
      numPagesAllocated--;
    }
  }
}



//function to get a pageframe to replace
int
ProcessAddrSpace::replacePage(int ign){
      int tmp;
      if(pageReplaceAlgo==1){
          tmp = rand()% NumPhysPages;
          while (tmp == ign || machine->PhysMap[tmp].IsShared )
            tmp = rand()% NumPhysPages;
          return tmp;
      }
      if(pageReplaceAlgo==2){
				printf("replace 2\n");
				void* temp = fifoList->Remove();
				//*((int*)temp->item)
        tmp = *((int*)temp);
				printf("replace 2 %d\n", tmp);

        while(tmp == ign)
        {
					//printf("1 replace 2\n");
					int *x = new int[1];
					*x = tmp;
					printf("accessible %d\n", *x);
					fifoList->Append((void *)x);
					//printf("2 replace 2\n");

					tmp = *((int*)fifoList->Remove());
					//printf(" 3 replace 2\n");

        }
				printf(" 4 replace 2 :  %d\n ",tmp);

        return tmp;

			}
      if(pageReplaceAlgo==3){
        tmp = *((int*)lruList->Remove());
        if(tmp == ign)
        {
					int *x = new int[1];
					*x = tmp;
					printf("replace --------------- %d\n", *x);
          lruList->Append((void *)x);
          tmp = *((int*)lruList->Remove());
        }
				int *x = new int[1];
				*x = tmp;
				printf("REPLACE - Appening  %d to the list NumPageFaults :%d\n", *x, NumPageFaults);
				lruList->Append((void *)x);
        return tmp;
      }
      if(pageReplaceAlgo==4){
            while(machine->Refbit[headpt] == TRUE || machine->PhysMap[headpt].IsShared || headpt == ign ){
							printf("replacePage %d  head: %d\n", numPagesAllocated, headpt);

								machine->Refbit[headpt] = FALSE;
                if(headpt == ign)
                  machine->Refbit[headpt] = TRUE;
                headpt = (headpt+1)%NumPhysPages;
            }
						printf("replacePage %d\n", numPagesAllocated);
            machine->Refbit[headpt]= TRUE;
						headpt = (headpt+1)%NumPhysPages;
            return (headpt-1 +NumPhysPages)%NumPhysPages;
      }
}





//function to call while accessing a pageFrame
void
ProcessAddrSpace::access(int pageFrame){
  if(machine->PhysMap[pageFrame].IsShared == FALSE){
      if(pageReplaceAlgo==2){
			///	printf("access 2\n");
            ListElement *temp = fifoList->first;
				//		printf("1 access 2\n");
				if(temp !=NULL)
					//printf("First element :  %d\n",*((int*)temp->item) );
            while(temp!=NULL ){
                if(*((int*)temp->item) == pageFrame) break;
                temp = temp->next;
            }
            if(temp==NULL){
							//printf("Accesing %d\n",pageFrame );
							int *x = new int[1];
							*x = pageFrame;
							printf("accessible %d\n", *x);
              fifoList->Append((void *)x);
            }
      }

      if(pageReplaceAlgo==3){
            ListElement *temp = lruList->first;

						bool found = FALSE;
            if(temp !=NULL){
							//printf("access num :%d pageFrame: %d\n", numPagesAllocated, pageFrame);
							printf("First element is %d searching for :%d\n", *((int*)temp->item),pageFrame);

	            if(*((int*)temp->item) == pageFrame)
							{

								found = TRUE;
								lruList->first = temp->next;
								if(found && (temp->next != NULL))
																	printf("First element is now  %d searching for:\n", *((int*)temp->next->item),pageFrame);

							}
							else{
	            while(temp->next!=NULL && found == FALSE){
								printf("Iterating now over :%d  searching for:%d NumPageFaults : %d\n", *((int*)temp->next->item),pageFrame, NumPageFaults);

									if(*((int*)temp->next->item) == pageFrame)
	                      found = TRUE;
									else temp = temp->next;
	            }

	            if(found)
							{
								if(temp->next != NULL)
								{
									printf("Removing element %d\n", *((int*)temp->next->item) );
									temp->next = temp->next->next;

									if(temp->next == NULL)
										lruList->last = temp;
									if(temp->next != NULL)
									printf("Next  element is now %d\n", *((int*)temp->next->item) );
								}
								else{
									temp->next = NULL;

								}
							}

	            }
						}
						int *x = new int[1];
						*x = pageFrame;
						printf("Appening  %d to the list NumPageFaults :%d\n", *x, NumPageFaults);
						lruList->Append((void *)x);

      }
      if(pageReplaceAlgo==4){
				printf("access numPagesAllocated: %d head: %d pageFrame: %d NumPageFaults: %d\n", numPagesAllocated, headpt, pageFrame, NumPageFaults);

          machine->Refbit[pageFrame]=TRUE;
        //  headpt = (headpt+1)%NumPhysPages;
      }
    }
}


//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
void
ProcessAddrSpace::LoadPage(int vpn)
{
    ASSERT(NachOSpageTable == machine->NachOSpageTable);

		OpenFile *executable = fileSystem->Open(currentThread->fileName);
		if(executable == NULL)
		{
				printf("Unable to open file %s\n", currentThread->fileName);
				return;
	  }

		int i;
    int ppn = PagetoEvict(-1);
	//	printf("(LoadPage) ppn= %d vpn = %d pid= %d\n", ppn, vpn, currentThread->GetPID());

    if(NachOSpageTable[vpn].swapped)
    	for(i=0;i<PageSize;i++)
      	machine->mainMemory[ppn*PageSize+i] = currentThread->SwapTable[vpn*PageSize+i];
    else
		{
				NoffHeader noffH;
				executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
		    if ((noffH.noffMagic != NOFFMAGIC) &&
				(WordToHost(noffH.noffMagic) == NOFFMAGIC))
		    	SwapHeader(&noffH);
		    ASSERT(noffH.noffMagic == NOFFMAGIC);

				//code_pn = (noffH.code.virtualAddr + PageSize -1)/PageSize;
				//initData_pn = (noffH.initData.virtualAddr + PageSize -1)/PageSize;
				executable->ReadAt(&(machine->mainMemory[ppn*PageSize]), PageSize, noffH.code.inFileAddr+vpn*PageSize);
				/*  Buggy
				if (noffH.code.size > 0 && (noffH.code.inFileAddr + vpn*PageSize) <= noffH.code.size) {
					printf("Inside noffH.code\n");
		      //  DEBUG('a', "Initializing code segment, at 0x%x, size %d\n",
					//noffH.code.virtualAddr, noffH.code.size);
		      vpn = noffH.code.virtualAddr/PageSize;
		        offset = noffH.code.virtualAddr%PageSize;
		        entry = &NachOSpageTable[vpn];
		        pageFrame = entry->physicalPage;
		        executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
					noffH.code.size, noffH.code.inFileAddr);
		    }
		    else if (noffH.initData.size > 0 && (noffH.initData.inFileAddr + vpn*PageSize) <= noffH.initData.size) {
					printf("Inside noffH.initData\n");
		      //  DEBUG('a', "Initializing data segment, at 0x%x, size %d\n",
					//noffH.initData.virtualAddr, noffH.initData.size);
		        vpn = noffH.initData.virtualAddr/PageSize;
		        offset = noffH.initData.virtualAddr%PageSize;
		        entry = &NachOSpageTable[vpn];
		        pageFrame = entry->physicalPage;
		      //  executable->ReadAt(&(machine->mainMemory[pageFrame * PageSize + offset]),
				//	noffH.initData.size, noffH.initData.inFileAddr);
					executable->ReadAt(&(machine->mainMemory[ppn*PageSize]), PageSize, noffH.initData.inFileAddr+vpn*PageSize);
		    }
				else
				printf("@@@@@@@@@@@@@@@@@@@22\n");*/
		}

    NachOSpageTable[vpn].physicalPage = ppn;
    NachOSpageTable[vpn].valid = TRUE;
    NachOSpageTable[vpn].swapped = FALSE;
    NachOSpageTable[vpn].use = TRUE;
    NachOSpageTable[vpn].dirty = FALSE;
    NachOSpageTable[vpn].shared = FALSE;

  	machine->PhysMap[ppn].processID = currentThread->GetPID();
    machine->PhysMap[ppn].virtPage = vpn;
    machine->PhysMap[ppn].IsShared = FALSE;
		machine->PhysMap[ppn].IsEmpty = FALSE;

		delete executable;
}

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
int
ProcessAddrSpace::PagetoEvict(int ign)
{
    int i=0;
		//printf("(PagetoEvict)\n");
    if(numPagesAllocated < NumPhysPages)
    {
      numPagesAllocated++;
      while(machine->PhysMap[i].IsEmpty==FALSE && !(machine->PhysMap[i].IsShared) && i < NumPhysPages)	i++;
      return i;
    }
    else
    {
      int ppn =  replacePage(ign);
			printf("PPN : %d\n", ppn );
      int vpage = machine->PhysMap[ppn].virtPage;
			printf("VPAGE : %d\n", vpage );

			TranslationEntry *Table = threadArray[machine->PhysMap[ppn].processID]->space->GetPageTable();
			//printf("234567890-87654330\n");

			Table[vpage].valid = FALSE;
			DEBUG('p', "Page no. %d swapped out: \n",ppn);
			if(Table[vpage].dirty)
			{
				Table[vpage].swapped =TRUE;
				for(i=0;i<PageSize;i++)

					threadArray[machine->PhysMap[ppn].processID]->SwapTable[vpage*PageSize+i] = machine->mainMemory[ppn*PageSize+i];

		}//return PageReplaceAlgo();
		//printf("!!@@@!#@@\n");
      return ppn;
    }
}


//----------------------------------------------------------------------
// ProcessAddrSpace::InitUserCPURegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
ProcessAddrSpace::InitUserCPURegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPagesInVM * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPagesInVM * PageSize - 16);
}

//----------------------------------------------------------------------
// ProcessAddrSpace::SaveStateOnSwitch
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void ProcessAddrSpace::SaveStateOnSwitch()
{}

//----------------------------------------------------------------------
// ProcessAddrSpace::RestoreStateOnSwitch
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void ProcessAddrSpace::RestoreStateOnSwitch()
{
    machine->NachOSpageTable = NachOSpageTable;
    machine->NachOSpageTableSize = numPagesInVM;
}

unsigned
ProcessAddrSpace::GetNumPages()
{
   return numPagesInVM;
}

TranslationEntry*
ProcessAddrSpace::GetPageTable()
{
   return NachOSpageTable;
}
