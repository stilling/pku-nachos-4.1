// main.h 
//	This file defines the Nachos global variables
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef MAIN_H
#define MAIN_H
#define MaxThreads 128
#include "copyright.h"
#include "debug.h"
#include "kernel.h"
class Kernel;
extern Kernel *kernel;
extern Debug *debug;
//int ThreadIDs[MaxThreads];


#endif // MAIN_H
// Limit the number of threads
#define MaxThreads 128
//是否使用TLB
#define USE_TLB 1
//bitmap
#define USER_PROGRAM
#ifdef USER_PROGRAM
#include "bitmap.h"
#include "machine.h"
extern Bitmap* memoryBitMap;
extern Machine* machine;

#endif

// #ifdef FILESYS_STUB
// #undef FILESYS_STUB
// #endif 
#ifndef FILESYS_STUB
#define USE_TLB
#endif 