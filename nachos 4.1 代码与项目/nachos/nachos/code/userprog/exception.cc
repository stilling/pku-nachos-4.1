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
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
#include "machine.h"
#include "interrupt.h"
#include "openfile.h"
#include "addrspace.h"
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
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	is in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
	int type = kernel->machine->ReadRegister(2);
	if(which==2) int pageaddr=kernel->machine->ReadRegister(BadVAddrReg);
	DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");
	switch(which)
	{
	case SyscallException:
      
      if(type==SC_Halt){
	DEBUG(dbgSys, "Shutdown, initiated by user program.\n");

	SysHalt();
	kernel->interrupt->Halt();
	return;
	ASSERTNOTREACHED();
	}
		else if(type==SC_Exit)
		{
		cout<<"user programm exit"<<endl;
		//printf("Page %d is clear  kernel->machine->pageTableSize%d\n",kernel->machine->pageTable[12].physicalPage,kernel->machine->pageTableSize);
		for(int i=0;i<kernel->machine->pageTableSize;i++)
		{
memoryBitMap->Clear(kernel->machine->pageTable[i].physicalPage);
printf("Page %d is clear\n",kernel->machine->pageTable[i].physicalPage);
		}
		
		int NextPC=kernel->machine->ReadRegister(NextPCReg);

		kernel->machine->WriteRegister(PCReg,NextPC);
		delete kernel->currentThread->space;
		kernel->currentThread->space=NULL;
		kernel->currentThread->Finish();
		return;
		}
      
		if (type==SC_Create)
{
		int viradress=kernel->machine->ReadRegister(4);
		int base=viradress;
		
		int value;int count=0;
		char filenameStr[128];
		do
		{
			kernel->machine->ReadMem(base+count,1,&value);
			filenameStr[count]=*(char*)&value;
			count++;
		}while(*(char*)&value!='\0'&&count<128);
		
		if(!kernel->fileSystem->Create(filenameStr))
		{
			cout<<"Create file failed!\n"<<endl;
		}
		else
			cout<<"Create file "<<filenameStr<<" success!"<<endl;
		kernel->machine->PCadvance();
		return;
	}
		else if(type==SC_Open)
	{
		
		
		int baseo=kernel->machine->ReadRegister(4);
		
		int valueo=0;int counto=0;
		char filenameStro[128];
		do
		{
			kernel->machine->ReadMem(baseo+counto,1,&valueo);
			filenameStro[counto]=*(char*)&valueo;
			counto++;
		}while(*(char*)&valueo!='\0'&&counto<128);
		int fid=SysOpen(filenameStro);
		if(fid<1)
		{
			cout<<"Open file "<<filenameStro<<" failed!\n"<<endl;
		}
		else
			cout<<"Open file "<<filenameStro<<" success!"<<endl;
		kernel->machine->PCadvance();
		return;
		}
		else if(type==SC_Close)
		{
		int cfileid;
		cfileid=kernel-machine->ReadRegister(4);
		if(SysClose(cfileid))
		{
			cout<<"Close file "<<cfileid<<" success!\n"<<endl;
			kernel->machine->WriteRegister(2,1);
		}
		else
		{
			cout<<"Close file "<<cfileid<<" failed!\n"<<endl;
			kernel->machine->WriteRegister(2,-1);
		}
		kernel->machine->PCadvance();
		return;
		}
		else if(type==SC_Write)

	{		
		int wbase,wsize,wvalue,wfileId,wcount;
		wbase=kernel-machine->ReadRegister(4);
		wsize=kernel-machine->ReadRegister(5);
		wfileId=kernel-machine->ReadRegister(6);
		wcount=0;
		char wfilenameStr[128];
		do
		{
			kernel->machine->ReadMem(wbase+wcount,1,&wvalue);
			wfilenameStr[wcount]=*(char*)&wvalue;
			wcount++;
		}while(*(char*)&wvalue!='\0'&&wcount<128);
		wfilenameStr[wsize]='\0';
		wcount=SysWrite(wfilenameStr,wsize,wfileId);
		if(wcount>-1)
		{
			printf("write at %s success\n",wfilenameStr);
			kernel->machine->WriteRegister(2,wcount);
		}
		else
		{
			printf("write at %s failed\n",wfilenameStr);
			kernel->machine->WriteRegister(2,1);
		}
		kernel->machine->PCadvance();
		return;
		ASSERTNOTREACHED();
	}

		else if(type== SC_Read)
	{
		int rbase,rsize,rvalue,rfileId,rcount;
		rbase=kernel-machine->ReadRegister(4);
		rsize=kernel-machine->ReadRegister(5);
		rfileId=kernel-machine->ReadRegister(6);
		rcount=0;
		char rfilenameStr[128];
		rcount=SysWrite(rfilenameStr,rsize,rfileId);
		if(rcount)
		{
			 for(int k=0;k<rcount;k++)
			 	kernel-machine->WriteMem(rbase+k,1,(int)rfilenameStr[k]);
			 cout<<"read file success, length is"<<rcount<<"content is"<<rfilenameStr<<endl;
		}
		else
		{
			cout<<"read file failed"<<endl;
		}
		kernel->machine->PCadvance();
		return;
	}
		

		else if(type== SC_ThreadFork)
{		cout<<"ThreadFork"<<endl;
		ASSERT(1>2);
		return;}
	else if(type==SC_ThreadYield)
{
		cout<<"current ThreadYield"<<endl;
		return;}
	else if(type==SC_ThreadJoin)
		{return;}
	else if(type==SC_ThreadExit)
	{return;}

	else if(type==SC_Add)
	{DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");
	
	/* Process SysAdd Systemcall*/
	int result;
	result = SysAdd(/* int op1 */(int)kernel->machine->ReadRegister(4),
			/* int op2 */(int)kernel->machine->ReadRegister(5));

	DEBUG(dbgSys, "Add returning with " << result << "\n");
	/* Prepare Result */
	kernel->machine->WriteRegister(2, (int)result);
	
	/* Modify return point */
	{
	  /* set previous programm counter (debugging only)*/
	  kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

	  /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
	  kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);
	  
	  /* set next programm counter for brach execution */
	  kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
	}

	
	
	ASSERTNOTREACHED();
}
else
	cerr << "Unexpected system call " << type << "\n";
	

      ASSERTNOTREACHED();
      return;
      break;
  	case PageFaultException:
  		cout<<"handle PageFaultException "<<endl;
  		//int pageaddr=kernel->machine->ReadRegister(BadVAddrReg);
		kernel->machine->PageFaultMiss(kernel->machine->ReadRegister(BadVAddrReg));
		//ASSERT(1>2);
		return;
    	break;
    case AddressErrorException:
    	printf("address falut\n");
    	return;
    	break;
    case TLBFaultException:
    	int addr=kernel->machine->ReadRegister(BadVAddrReg);
    	kernel->machine->TLBMiss(addr);
    	return;
    	break;
    default:
      cerr << "Unexpected user mode exception" << (int)which << "\n";
      printf("other falut\n");
      break;
    }
	
	

    cout<<"error exception "<<which<<endl;
    ASSERTNOTREACHED();
}
