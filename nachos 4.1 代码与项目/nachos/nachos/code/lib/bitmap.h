// bitmap.h 
//	Data structures defining a bitmap -- an array of bits each of which
//	can be either on or off.
//
//	Represented as an array of unsigned integers, on which we do
//	modulo arithmetic to find the bit we are interested in.
//
//	The bitmap can be parameterized with with the number of bits being 
//	managed.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef BITMAP_H
#define BITMAP_H

#include "copyright.h"
#include "utility.h"

// Definitions helpful for representing a bitmap as an array of integers
//eight byte behalf 128
const int BitsInByte =	8;
const int BitsInWord = sizeof(unsigned int) * BitsInByte;

// The following class defines a "bitmap" -- an array of bits,
// each of which can be independently set, cleared, and tested.
//
// Most useful for managing the allocation of the elements of an array --
// for instance, disk sectors, or main memory pages.
// Each bit represents whether the corresponding sector or page is
// in use or free.

class Bitmap {
  public:
    Bitmap(int numItems);	// Initialize a bitmap, with "numItems" bits
				// initially, all bits are cleared.
    //给出位图的大小，初始化位图
    ~Bitmap();			// De-allocate bitmap
    
    void Mark(int which);   	// Set the "nth" bit，标志which位被占用
    void Clear(int which);  	// Clear the "nth" bit，清除标志
    bool Test(int which) const;	// Is the "nth" bit set?测试是否被占用
    int FindAndSet();         // Return the # of a clear bit, and as a side
				// effect, set the bit. 找到第一个未被占用的位设置为占用
				// If no bits are clear, return -1.
    int FindAndAlloc();
    int NumClear() const;	// Return the number of clear bits
                            //返回多少位没有被占用
    void Print() const;		// Print contents of bitmap
    void SelfTest();		// Test whether bitmap is working
    
  protected:
    int numBits;		// number of bits in the bitmap
    int numWords;		// number of words of bitmap storage
				// (rounded up if numBits is not a
				//  multiple of the number of bits in
				//  a word)
    unsigned int *map;		// bit storage
};

#endif // BITMAP_H
