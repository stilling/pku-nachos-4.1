// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables.
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Once we'e implemented one set of higher level atomic operations,
// we can implement others using that implementation.  We illustrate
// this by implementing locks and condition variables on top of 
// semaphores, instead of directly enabling and disabling interrupts.
//
// Locks are implemented using a semaphore to keep track of
// whether the lock is held or not -- a semaphore value of 0 means
// the lock is busy; a semaphore value of 1 means the lock is free.
//
// The implementation of condition variables using semaphores is
// a bit trickier, as explained below under Condition::Wait.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "main.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List<Thread *>;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    Interrupt *interrupt = kernel->interrupt;
    Thread *currentThread = kernel->currentThread;
    
    // disable interrupts
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	
    
    while (value == 0) { 		// semaphore not available
	queue->Append(currentThread);	// so go to sleep
	currentThread->Sleep(FALSE);
    } 
    value--; 			// semaphore available, consume its value
   
    // re-enable interrupts
    (void) interrupt->SetLevel(oldLevel);	
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that interrupts
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Interrupt *interrupt = kernel->interrupt;
    
    // disable interrupts
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	
    
    if (!queue->IsEmpty()) {  // make thread ready.
	kernel->scheduler->ReadyToRun(queue->RemoveFront());
    }
    value++;
    
    // re-enable interrupts
    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Semaphore::SelfTest, SelfTestHelper
// 	Test the semaphore implementation, by using a semaphore
//	to control two threads ping-ponging back and forth.
//----------------------------------------------------------------------

static Semaphore *ping;
static void
SelfTestHelper (Semaphore *pong) 
{
    for (int i = 0; i < 10; i++) {
        ping->P();
	pong->V();
    }
}

void
Semaphore::SelfTest()
{
    Thread *helper = new Thread("ping");

    ASSERT(value == 0);		// otherwise test won't work!
    ping = new Semaphore("ping", 0);
    helper->Fork((VoidFunctionPtr) SelfTestHelper, this);
    for (int i = 0; i < 10; i++) {
        ping->V();
	this->P();
    }
    delete ping;
}

//----------------------------------------------------------------------
// Lock::Lock
// 	Initialize a lock, so that it can be used for synchronization.
//	Initially, unlocked.
//
//	"debugName" is an arbitrary name, useful for debugging.
//----------------------------------------------------------------------

Lock::Lock(char* debugName)
{
    name = debugName;
    semaphore = new Semaphore("lock", 1);  // initially, unlocked
    lockHolder = NULL;
}

//----------------------------------------------------------------------
// Lock::~Lock
// 	Deallocate a lock
//----------------------------------------------------------------------
Lock::~Lock()
{
    delete semaphore;
}

//----------------------------------------------------------------------
// Lock::Acquire
//	Atomically wait until the lock is free, then set it to busy.
//	Equivalent to Semaphore::P(), with the semaphore value of 0
//	equal to busy, and semaphore value of 1 equal to free.
//----------------------------------------------------------------------

void Lock::Acquire()
{
    semaphore->P();
    lockHolder = kernel->currentThread;
}

//----------------------------------------------------------------------
// Lock::Release
//	Atomically set lock to be free, waking up a thread waiting
//	for the lock, if any.
//	Equivalent to Semaphore::V(), with the semaphore value of 0
//	equal to busy, and semaphore value of 1 equal to free.
//
//	By convention, only the thread that acquired the lock
// 	may release it.
//---------------------------------------------------------------------

void Lock::Release()
{
    ASSERT(IsHeldByCurrentThread());
    lockHolder = NULL;
    semaphore->V();
}

//----------------------------------------------------------------------
// Condition::Condition
// 	Initialize a condition variable, so that it can be 
//	used for synchronization.  Initially, no one is waiting
//	on the condition.
//
//	"debugName" is an arbitrary name, useful for debugging.
//----------------------------------------------------------------------
Condition::Condition(char* debugName)
{
    name = debugName;
    waitQueue = new List<Semaphore *>;
}

//----------------------------------------------------------------------
// Condition::Condition
// 	Deallocate the data structures implementing a condition variable.
//----------------------------------------------------------------------

Condition::~Condition()
{
    delete waitQueue;
}

//----------------------------------------------------------------------
// Condition::Wait
// 	Atomically release monitor lock and go to sleep.
//	Our implementation uses semaphores to implement this, by
//	allocating a semaphore for each waiting thread.  The signaller
//	will V() this semaphore, so there is no chance the waiter
//	will miss the signal, even though the lock is released before
//	calling P().
//
//	Note: we assume Mesa-style semantics, which means that the
//	waiter must re-acquire the monitor lock when waking up.
//
//	"conditionLock" -- lock protecting the use of this condition
//----------------------------------------------------------------------

void Condition::Wait(Lock* conditionLock) 
{
     Semaphore *waiter;
    
     ASSERT(conditionLock->IsHeldByCurrentThread());

     waiter = new Semaphore("condition", 0);
     waitQueue->Append(waiter);
     conditionLock->Release();
     waiter->P();
     conditionLock->Acquire();
     delete waiter;
}

//----------------------------------------------------------------------
// Condition::Signal
// 	Wake up a thread waiting on this condition, if any.
//
//	Note: we assume Mesa-style semantics, which means that the
//	signaller doesn't give up control immediately to the thread
//	being woken up (unlike Hoare-style).
//
//	Also note: we assume the caller holds the monitor lock
//	(unlike what is described in Birrell's paper).  This allows
//	us to access waitQueue without disabling interrupts.
//
//	"conditionLock" -- lock protecting the use of this condition
//----------------------------------------------------------------------

void Condition::Signal(Lock* conditionLock)
{
    Semaphore *waiter;
    
    ASSERT(conditionLock->IsHeldByCurrentThread());
    
    if (!waitQueue->IsEmpty()) {
        waiter = waitQueue->RemoveFront();
	waiter->V();
    }
}

//----------------------------------------------------------------------
// Condition::Broadcast
// 	Wake up all threads waiting on this condition, if any.
//
//	"conditionLock" -- lock protecting the use of this condition
//----------------------------------------------------------------------

void Condition::Broadcast(Lock* conditionLock) 
{
    while (!waitQueue->IsEmpty()) {
        Signal(conditionLock);
    }
}
// 信号量解决生产者消费者问题
#define N 1024 // 缓冲区大小
Semaphore* empty = new Semaphore("emptyBuffer", N);
Semaphore* mutex = new Semaphore("lockSemaphore", 1);
Semaphore* full = new Semaphore("fullBuffer", 0);
int msgQueue = 0;

void Producer(int val) {
	while (1) {
		empty->P();
		mutex->P();
		if (msgQueue >= N) { // 已经满了则停止生产
			printf("-->Product alread full:[%d],wait consumer.", msgQueue);
		}
		else {
			printf("-->name:[%s],threadId:[%d],before:[%d],after:[%d]\n", \
				currentThread->getName(), currentThread->getThreadId(), msgQueue, msgQueue + 1);
			++msgQueue;
		}
		mutex->V();
		full->V();

		sleep(val); // 休息下再生产
	}
}

void Customer(int val) {
	while (1) {
		full->P();
		mutex->P();
		if (msgQueue <= 0) {
			printf("Product alread empty:[%d],wait Producer.", msgQueue);
		}
		else {
			printf("name:[%s] threadId:[%d],before:[%d],after:[%d]\n", \
				currentThread->getName(), currentThread->getThreadId(), msgQueue, msgQueue - 1);
			--msgQueue;
		}
		mutex->V();
		empty->V();

		sleep(val); // 休息下再消费
	}
}

void ThreadProducerConsumerTest() {
	DEBUG('t', "Entering ThreadProducerConsumerTest");
	// 两个生产者
	Thread* p1 = new Thread("Producer1");
	Thread* p2 = new Thread("Producer2");
	p1->Fork(Producer, 1);
	p2->Fork(Producer, 3);

	// 两个消费者，可以关掉一个消费者，查看生产速率和消费速率的变化
	Thread* c1 = new Thread("Consumer1");
	//Thread* c2 = new Thread("Consumer2");
	c1->Fork(Customer, 1);
	//c2->Fork(Customer, 2);
}

// 条件变量实现barrier
Condition* barrCond = new Condition("BarrierCond");
Lock* barrLock = new Lock("BarrierLock");
int barrierCnt = 0;
// 当且仅当barrierThreadNum个线程同时到达时才能往下运行
const int barrierThreadNum = 5;

void barrierFun(int num)
{
	/*while(1)*/
	{
		barrLock->Acquire();
		++barrierCnt;

		if (barrierCnt == barrierThreadNum) {
			// 最后一个线程到达后判断，条件满足则发送一个广播信号
			// 唤醒等待在该条件变量上的所有线程
			printf("threadName:[%s%d],barrierCnt:[%d],needCnt:[%d],Broadcast.\n", \
				currentThread->getName(), num, barrierCnt, barrierThreadNum);
			barrCond->Broadcast(barrLock);
			barrLock->Release();
		}
		else {
			// 每一个线程都执行判断，若条件不满足，线程等待在条件变量上
			printf("threadName:[%s%d],barrierCnt:[%d],needCnt:[%d],Wait.\n", \
				currentThread->getName(), num, barrierCnt, barrierThreadNum);
			barrCond->Wait(barrLock);
			barrLock->Release();
		}
		printf("threadName:[%s%d],continue to run.\n", currentThread->getName(), num);
	}
}

void barrierThreadTest() {
	DEBUG('t', "Entering barrierThreadTest");
	for (int i = 0; i < barrierThreadNum; ++i) {
		Thread* t = new Thread("barrierThread");
		t->Fork(barrierFun, i + 1);
	}
}

int rCnt = 0; // 记录读者数量
Lock* rLock = new Lock("rlock");
// 必须用信号量，不能用锁，因为锁只能由加锁的线程解锁
Semaphore* wLock = new Semaphore("wlock", 1);
int bufSize = 0;
// Lab3 锁实现读者写者问题
void readFunc(int num) {
	while (1) {
		rLock->Acquire();
		++rCnt;
		// 如果是第一个读者进入，需要竞争1值信号量wLock，竞争成功才能进入临界区
		// 一旦竞争到wLock，由最后一个读者出临界区后释放，保证了读者优先
		if (rCnt == 1) {
			wLock->P();
		}
		rLock->Release();
		if (0 == bufSize) {
			// 没有数据可读
			printf("threadName:[%s],bufSize:[%d],current not data.\n", currentThread->getName(), bufSize);
		}
		else {
			// 读取数据
			printf("threadName:[%s],bufSize:[%d],exec read operation.\n", currentThread->getName(), bufSize);
		}
		rLock->Acquire();
		--rCnt;
		// 最后一个读者释放wLock
		if (rCnt == 0) {
			wLock->V();
		}
		rLock->Release();
		currentThread->Yield();
		sleep(num);
	}
}

void writeFunc(int num) {
	while (1) {
		wLock->P();
		++bufSize;
		printf("writerThread:[%s],before:[%d],after:[%d]\n", currentThread->getName(), bufSize, bufSize + 1);
		wLock->V();
		currentThread->Yield();
		sleep(num);
	}
}

void readWriteThreadTest()
{
	DEBUG('t', "Entering readWriteThreadTest");
	Thread * r1 = new Thread("read1");
	Thread * r2 = new Thread("read2");
	Thread * r3 = new Thread("read3");
	Thread * w1 = new Thread("write1");
	Thread * w2 = new Thread("write2");

	// 3个读者2个写者
	r1->Fork(readFunc, 1);
	w1->Fork(writeFunc, 1);
	r2->Fork(readFunc, 1);
	w2->Fork(writeFunc, 1);
	r3->Fork(readFunc, 1);
}