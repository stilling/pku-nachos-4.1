// thread.cc 
//	Routines to manage threads.  These are the main operations:
//
//	Fork -- create a thread to run a procedure concurrently
//		with the caller (this is done in two steps -- first
//		allocate the Thread object, then call Fork on it)
//	Begin -- called when the forked procedure starts up, to turn
//		interrupts on and clean up after last thread
//	Finish -- called when the forked procedure finishes, to clean up
//	Yield -- relinquish control over the CPU to another ready thread
//	Sleep -- relinquish control over the CPU, but thread is now blocked.
//		In other words, it will not run again, until explicitly 
//		put back on the ready queue.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.
#include <unistd.h>
#include "copyright.h"
#include "thread.h"
#include "switch.h"
#include "synch.h"
#include "sysdep.h"
#include "stdio.h"
extern int ThreadIDs[MaxThreads];
// this is put at the top of the execution stack, for detecting stack overflows
const int STACK_FENCEPOST = 0xdedbeef;
//----------------------------------------------------------------------
// int
// Thread::getUserID()
// { return UserID; }

// int
// Thread::getThreadID()
// { return ThreadID; } 

int
Thread::AllocateThreadID()
{
	
	int i ;
	for(i = 0 ; i < MaxThreads; i++)
	{
		if(ThreadIDs[i] == 0)
		{
			break;
		}
	}
	if(i < MaxThreads)
		{
			ThreadIDs[i]=1;		
			return i+1;
		}
	else
		return -1;
}

//----------------------------------------------------------------------
// Thread::Thread
// 	Initialize a thread control block, so that we can then call
//	Thread::Fork.
//
//	"threadName" is an arbitrary string, useful for debugging.
//----------------------------------------------------------------------

Thread::Thread(char* threadName)
{
    name = threadName;
	UserID = getuid();
	ThreadID  = AllocateThreadID();
    stackTop = NULL;
    stack = NULL;
    status = JUST_CREATED;
    priority = 0;
	if(ThreadID == -1) printf("Reached MaxThreadsNum !! \n");
	ASSERT(ThreadID != -1)
    for (int i = 0; i < MachineStateSize; i++) {
	machineState[i] = NULL;		// not strictly necessary, since
					// new thread ignores contents 
					// of machine registers
    }
    space = NULL;
}

Thread::Thread(char* threadName,int p)
{
	name = threadName;
	UserID = getuid();
	ThreadID  = AllocateThreadID();
    stackTop = NULL;
    stack = NULL;
    status = JUST_CREATED;
    priority = p;
	if(ThreadID == -1) printf("Reached MaxThreadsNum !! \n");
	ASSERT(ThreadID != -1)
    for (int i = 0; i < MachineStateSize; i++) {
	machineState[i] = NULL;		// not strictly necessary, since
					// new thread ignores contents 
					// of machine registers
    }
    space = NULL;
}


//----------------------------------------------------------------------
// Thread::~Thread
// 	De-allocate a thread.
//
// 	NOTE: the current thread *cannot* delete itself directly,
//	since it is still running on the stack that we need to delete.
//
//      NOTE: if this is the main thread, we can't delete the stack
//      because we didn't allocate it -- we got it automatically
//      as part of starting up Nachos.
//----------------------------------------------------------------------

Thread::~Thread()
{
    DEBUG(dbgThread, "Deleting thread: " << name);

    ASSERT(this != kernel->currentThread);
    if (stack != NULL)
	DeallocBoundedArray((char *) stack, StackSize * sizeof(int));
	ThreadIDs[ThreadID] == 0;
}

//----------------------------------------------------------------------
// Thread::Fork
// 	Invoke (*func)(arg), allowing caller and callee to execute 
//	concurrently.
//
//	NOTE: although our definition allows only a single argument
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
Thread::Fork(VoidFunctionPtr func, void *arg)
{
    Interrupt *interrupt = kernel->interrupt;
    Scheduler *scheduler = kernel->scheduler;
    IntStatus oldLevel;
    
    DEBUG(dbgThread, "Forking thread: " << name << " f(a): " << (int) func << " " << arg);
    
    StackAllocate(func, arg);
    //func(1);
    oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this);	// ReadyToRun assumes that interrupts 
					// are disabled!
    (void) interrupt->SetLevel(oldLevel);
}    

//----------------------------------------------------------------------
// Thread::CheckOverflow
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
Thread::CheckOverflow()
{
    if (stack != NULL) {
#ifdef HPUX			// Stacks grow upward on the Snakes
	ASSERT(stack[StackSize - 1] == STACK_FENCEPOST);
#else
	ASSERT(*stack == STACK_FENCEPOST);
#endif
   }
}

//----------------------------------------------------------------------
// Thread::Begin
// 	Called by ThreadRoot when a thread is about to begin
//	executing the forked procedure.
//
// 	It's main responsibilities are:
//	1. deallocate the previously running thread if it finished 
//		(see Thread::Finish())
//	2. enable interrupts (so we can get time-sliced)
//----------------------------------------------------------------------

void
Thread::Begin ()
{
    ASSERT(this == kernel->currentThread);
    DEBUG(dbgThread, "Beginning thread: " << name);
    
    kernel->scheduler->CheckToBeDestroyed();
    kernel->interrupt->Enable();
}

//----------------------------------------------------------------------
// Thread::Finish
// 	Called by ThreadRoot when a thread is done executing the 
//	forked procedure.
//
// 	NOTE: we can't immediately de-allocate the thread data structure 
//	or the execution stack, because we're still running in the thread 
//	and we're still on the stack!  Instead, we tell the scheduler
//	to call the destructor, once it is running in the context of a different thread.
//
// 	NOTE: we disable interrupts, because Sleep() assumes interrupts
//	are disabled.
//----------------------------------------------------------------------

//
void
Thread::Finish ()
{
    (void) kernel->interrupt->SetLevel(IntOff);		
    ASSERT(this == kernel->currentThread);
    
    DEBUG(dbgThread, "Finishing thread: " << name);
    
    Sleep(TRUE);				// invokes SWITCH
    // not reached
}

//----------------------------------------------------------------------
// Thread::Yield
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
// 	Similar to Thread::Sleep(), but a little different.
//----------------------------------------------------------------------

void
Thread::Yield ()
{
    Thread *nextThread;
    IntStatus oldLevel = kernel->interrupt->SetLevel(IntOff);
    
    ASSERT(this == kernel->currentThread);
    
    DEBUG(dbgThread, "Yielding thread: " << name);
    
    nextThread = kernel->scheduler->FindNextToRun();
    if (nextThread != NULL) {
	kernel->scheduler->ReadyToRun(this);
	kernel->scheduler->Run(nextThread, FALSE);
    }
    (void) kernel->interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Thread::Sleep
// 	Relinquish the CPU, because the current thread has either
//	finished or is blocked waiting on a synchronization 
//	variable (Semaphore, Lock, or Condition).  In the latter case,
//	eventually some thread will wake this thread up, and put it
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
Thread::Sleep (bool finishing)
{
    Thread *nextThread;
    
    ASSERT(this == kernel->currentThread);
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    
    DEBUG(dbgThread, "Sleeping thread: " << name);

    status = BLOCKED;
    while ((nextThread = kernel->scheduler->FindNextToRun()) == NULL)
	kernel->interrupt->Idle();	// no one to run, wait for an interrupt
    
    // returns when it's time for us to run
    kernel->scheduler->Run(nextThread, finishing); 
}

//----------------------------------------------------------------------
// ThreadBegin, ThreadFinish,  ThreadPrint
//	Dummy functions because C++ does not (easily) allow pointers to member
//	functions.  So we create a dummy C function
//	(which we can pass a pointer to), that then simply calls the 
//	member function.
//----------------------------------------------------------------------

static void ThreadFinish()    { kernel->currentThread->Finish(); }
static void ThreadBegin() { kernel->currentThread->Begin(); }
void ThreadPrint(Thread *t) { 
	t->Print(); 
}

#ifdef PARISC

//----------------------------------------------------------------------
// PLabelToAddr
//	On HPUX, function pointers don't always directly point to code,
//	so we need to do the conversion.
//----------------------------------------------------------------------

static void *
PLabelToAddr(void *plabel)
{
    int funcPtr = (int) plabel;

    if (funcPtr & 0x02) {
        // L-Field is set.  This is a PLT pointer
        funcPtr -= 2;	// Get rid of the L bit
        return (*(void **)funcPtr);
    } else {
        // L-field not set.
        return plabel;
    }
}
#endif

//----------------------------------------------------------------------
// Thread::StackAllocate
//	Allocate and initialize an execution stack.  The stack is
//	initialized with an initial stack frame for ThreadRoot, which:
//		enables interrupts
//		calls (*func)(arg)
//		calls Thread::Finish
//
//	"func" is the procedure to be forked
//	"arg" is the parameter to be passed to the procedure
//----------------------------------------------------------------------

void
Thread::StackAllocate (VoidFunctionPtr func, void *arg)
{
    stack = (int *) AllocBoundedArray(StackSize * sizeof(int));

#ifdef PARISC
    // HP stack works from low addresses to high addresses
    // everyone else works the other way: from high addresses to low addresses
    stackTop = stack + 16;	// HP requires 64-byte frame marker
    stack[StackSize - 1] = STACK_FENCEPOST;
#endif

#ifdef SPARC
    stackTop = stack + StackSize - 96; 	// SPARC stack must contains at 
					// least 1 activation record 
					// to start with.
    *stack = STACK_FENCEPOST;
#endif 

#ifdef PowerPC // RS6000
    stackTop = stack + StackSize - 16; 	// RS6000 requires 64-byte frame marker
    *stack = STACK_FENCEPOST;
#endif 

#ifdef DECMIPS
    stackTop = stack + StackSize - 4;	// -4 to be on the safe side!
    *stack = STACK_FENCEPOST;
#endif

#ifdef ALPHA
    stackTop = stack + StackSize - 8;	// -8 to be on the safe side!
    *stack = STACK_FENCEPOST;
#endif


#ifdef x86
    // the x86 passes the return address on the stack.  In order for SWITCH() 
    // to go to ThreadRoot when we switch to this thread, the return addres 
    // used in SWITCH() must be the starting address of ThreadRoot.
    stackTop = stack + StackSize - 4;	// -4 to be on the safe side!
    *(--stackTop) = (int) ThreadRoot;
    *stack = STACK_FENCEPOST;
#endif
    
#ifdef PARISC
    machineState[PCState] = PLabelToAddr(ThreadRoot);
    machineState[StartupPCState] = PLabelToAddr(ThreadBegin);
    machineState[InitialPCState] = PLabelToAddr(func);
    machineState[InitialArgState] = arg;
    machineState[WhenDonePCState] = PLabelToAddr(ThreadFinish);
#else
    machineState[PCState] = (void*)ThreadRoot;
    machineState[StartupPCState] = (void*)ThreadBegin;
    machineState[InitialPCState] = (void*)func;
    machineState[InitialArgState] = (void*)arg;
    machineState[WhenDonePCState] = (void*)ThreadFinish;
#endif
}

#include "machine.h"

//----------------------------------------------------------------------
// Thread::SaveUserState
//	Save the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers -- 
//	one for its state while executing user code, one for its state 
//	while executing kernel code.  This routine saves the former.
//----------------------------------------------------------------------

void
Thread::SaveUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
	userRegisters[i] = kernel->machine->ReadRegister(i);
}

//----------------------------------------------------------------------
// Thread::RestoreUserState
//	Restore the CPU state of a user program on a context switch.
//
//	Note that a user program thread has *two* sets of CPU registers -- 
//	one for its state while executing user code, one for its state 
//	while executing kernel code.  This routine restores the former.
//----------------------------------------------------------------------

void
Thread::RestoreUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
	kernel->machine->WriteRegister(i, userRegisters[i]);
}


//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------
static void
ReadyPrint(int argc)
{
	cout<<"name:"<<kernel->currentThread->getName()<<"  tid:"<<kernel->currentThread->getThreadID()<<"  priority:"<<kernel->currentThread->getPriority()<<endl;
	//cout<<"fork"<<argc<<"time\n";
	kernel->currentThread->Yield();
	
}
static void
SimpleThread(int which)
{
    // int num;

    // for (num = 0; num < 20; num++) {
    //     cout << "*** testthread " << which <<"_"<<kernel->currentThread->getName()<<" priority="<<kernel->currentThread->getPriority()<< " already looped " << num << " times\n";//edit
    //     kernel->interrupt->OneTick();
    //     //kernel->currentThread->Yield();
    // }

}

//----------------------------------------------------------------------
// Thread::SelfTest
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
Thread::SelfTest()
{
    DEBUG(dbgThread, "Entering Thread::SelfTest");

    Thread *t1 = new Thread("thread1");
    Thread *t2 = new Thread("thread2");
    //Thread *t3 = new Thread("thread3");
    t1->setPriority(2);
    t2->setPriority(1);
    //t3->setPriority(3);
    // t1->Fork((VoidFunctionPtr) SimpleThread, (void *) 1);
    // t2->Fork((VoidFunctionPtr) SimpleThread, (void *) 2);
    t1->Fork((VoidFunctionPtr) ReadyPrint, (void *) 1);
    t2->Fork((VoidFunctionPtr) ReadyPrint, (void *) 2);
    //t3->Fork((VoidFunctionPtr) SimpleThread, (void *) 3);
    kernel->currentThread->Yield();
    //SimpleThread(0);
    ReadyPrint(0);
}
// void Mythreadprint(int arg)
// {
// 	Thread *t=(Thread*) arg;
// 	cout<<"  tid:"<<t->getThreadID()<<endl;
// }
// //输出就绪队列中全部线程
// void 
// ReadyListPrint(int what)
// {
// 	IntStatus oldLevel=interrupt->SetLevel(IntOff);
// 	printf("currentThread %s threadid %d \n",kernel->currentThread->getName\
// 	kernel->currentThread->getThreadID);
// 	List *list=new List();
// 	list=scheduler->getReadyList();
// 	if(!list->IsEmpty)
// 	{
// 		list->apply(Mythreadprint);
// 	}
// 	currentThread->Yeild();
// 	interrupt->SetLevel(oldLevel);
// } 	

// void
// threadtest()
// {
// 	//int cout = 0;
// 	DEBUG('t',"Entering threadTest");
// 	//printf("kernel thread: %s userid %d threadid %d \n",cout,kernel->currentThread->getName(),kernel->currentThread->getUserID(),kernel->currentThread->getThreadID());	
// 	// for(int i = 1; i <=MaxThreads;i++)
// 	// {
// 	// 	cout++; 
// 	// 	Thread *t = new Thread("test thread");
// 	// 	printf("time:%d tname %s userid %d threadid %d \n",cout,t->getName(),t->getUserID(),t->getThreadID());
// 	// }
// 	Thread *t1=new Thread("test thread 1");
// 	Thread *t2=new Thread("test thread 2");
// 	Thread *t3=new Thread("test thread 3");

// 	t1->Fork((VoidFunctionPtr) ReadyListPrint,0);
// 	t2->Fork((VoidFunctionPtr) ReadyListPrint,0);
// 	t3->Fork((VoidFunctionPtr) ReadyListPrint,0);
// }
void
Thread::LimitTest()
{
	
	int i;
	int cout=0;
	for(i=1;i<=MaxThreads;i++)
	{
		cout++;
		Thread *t=new Thread("Thread LimitTest");
		printf("time:%d tname %s userid %d threadid %d \n",cout,t->getName(),t->getUserID(),t->getThreadID());
	}
}


void
Thread::PrintTest()
{
	DEBUG('t',"Entering PrintTest");
	Thread *t1 = new Thread("thread1");
    Thread *t2 = new Thread("thread2");
    t1->Fork((VoidFunctionPtr) SimpleThread, (void *) 1);
    t2->Fork((VoidFunctionPtr) SimpleThread, (void *) 2);
    Interrupt *interrupt = kernel->interrupt;
	Scheduler *scheduler = kernel->scheduler;
    IntStatus oldLevel;
     oldLevel = interrupt->SetLevel(IntOff);
	 Print();
     scheduler->Print();	
     (void) interrupt->SetLevel(oldLevel);
	
} 

void
Thread::Print()
{
	//cout<<"name:"<<name<<"tid:"<<ThreadID<<endl;
	cout<<"name:"<<name<<"  tid:"<<ThreadID<<"  status:"<<ThreadStatusChar[status]<<"  priority"<<priority<<endl;
} 
