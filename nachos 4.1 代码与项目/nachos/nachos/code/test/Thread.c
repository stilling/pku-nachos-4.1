#include "syscall.h"

void func()
{
getThreadID();
ThreadYield();
}

int 
main()
{
getThreadID();
ThreadFork(func);
Halt();

}
