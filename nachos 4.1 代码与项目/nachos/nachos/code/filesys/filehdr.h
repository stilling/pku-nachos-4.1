// filehdr.h 
//	Data structures for managing a disk file header.  
//
//	A file header describes where on disk to find the data in a file,
//	along with other information about the file (for instance, its
//	length, owner, etc.)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "pbitmap.h"

#define NumDirect 	((SectorSize - 2 * sizeof(int)) / sizeof(int))
#define MaxFileSize 	(NumDirect * SectorSize)

// The following class defines the Nachos "file header" (in UNIX terms,  
// the "i-node"), describing where on disk to find all of the data in the file.
// The file header is organized as a simple table of pointers to
// data blocks. 
//
// The file header data structure can be stored in memory or on disk.
// When it is on disk, it is stored in a single sector -- this means
// that we assume the size of this data structure to be the same
// as one disk sector.  Without indirect addressing, this
// limits the maximum file length to just under 4K bytes.
//
// There is no constructor; rather the file header can be initialized
// by allocating blocks for the file (if it is a new file), or by
// reading it from disk.

class FileHeader {//i节点
  public:
    bool Allocate(PersistentBitmap *bitMap, int fileSize);// Initialize a file header, 
						//  including allocating space 
						//  on disk for the file data
    void Deallocate(PersistentBitmap *bitMap);  // De-allocate this file's 
						//  data blocks

    void FetchFrom(int sectorNumber); 	// Initialize file header from disk
    //从磁盘扇区中取出文件头，FetchFrom为将数据结构从磁盘读到内存的函数
    void WriteBack(int sectorNumber); 	// Write modifications to file header
					//  back to disk
//将文件头写入磁盘扇区
    int ByteToSector(int offset);	// Convert a byte offset into the file
					// to the disk sector containing
					// the byte 文件逻辑地址向物理地址转换

    int FileLength();			// Return the length of the file 
					// in bytes 返回文件长度

    void Print();			// Print the contents of the file.
    
    void setChangeTime();
    void setVisitTime(int sector);
    void setCreateTime();
  private:
    int numBytes;			// Number of bytes in the file 文件大小
    int numSectors;		// Number of data sectors in the file 占用扇区数
    int dataSectors[NumDirect];		// Disk sector numbers for each data 
					// block in the file 文件索引表
    //实验扩展文件属性（创建日期），间接索引扩充文件长度

    char createTime[25];
    char lastVisitTime[25];
    char lastChangeTime[25];
    
};

#endif // FILEHDR_H
