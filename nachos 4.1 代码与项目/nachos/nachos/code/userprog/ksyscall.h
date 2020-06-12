/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls 
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__ 
#define __USERPROG_KSYSCALL_H__ 

#include "kernel.h"




void SysHalt()
{
  kernel->interrupt->Halt();
}


int SysAdd(int op1, int op2)
{
  return op1 + op2;
}

int SysOpen(char *name)
{
	int fileid;
	OpenFile *openfile=kernel->fileSystem->Open(name);
	fileid=OpenForReadWrite(name, FALSE);
	if(!fileid) return -1;
	return fileid;
}

int SysWrite(char *buffer,int size,int id)
{
	OpenFile *openfile=new OpenFile(id);
	int wlength;
	wlength=openfile->Length();
	int num;
	num=openfile->WriteAt(buffer,size,wlength);
	if(num) return num;
	return -1;
}

int SysRead(char *buffer,int size,int id)
{
	OpenFile *openfile=new OpenFile(id);
	int readnum=0;
	if(readnum=openfile->Read(buffer,size))
	{
		return readnum;
	}
	return -1;

}

int SysClose(int fileid)
{
	if(!Close(fileid)) return -1;
	return 1;
}



#endif /* ! __USERPROG_KSYSCALL_H__ */
