// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

void
FileHeader::getType(char type[], char *name) 
{
	int p1, p;

	p1 = 0;
	p = 0;
	for (; name[p] != NULL && name[p] != '.'; p++);
	if (name[p] == NULL) 
		type[0] = '\0';
	else {
		p++;
		for (; name[p] != NULL; p++, p1++)
			type[p1] = name[p];
		type[p1] = '\0';
	}
}

void 
FileHeader::resetLastAccessTime()
{
	time_t timep;

	time(&timep);
	strncpy(lastAccessTime, asctime(localtime(&timep)), 25);
	lastAccessTime[24] = '\0';
}

void 
FileHeader::resetLastModifyTime()
{
	time_t timep;

	time(&timep);
	strncpy(lastModifyTime, asctime(localtime(&timep)), 25);
	lastModifyTime[24] = '\0';
}

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize, bool _isDir, int sector, char *name)
{ 
	time_t timep;
	char t[25];
	
	time(&timep);
    strncpy(t, asctime(localtime(&timep)), 25);
	t[24] = '\0';

	getType(type, name);
	strncpy(createTime, t, 25);
	createTime[24] = '\0';
	strncpy(lastAccessTime, t, 25);
	lastAccessTime[24] = '\0';
	strncpy(lastModifyTime, t, 25);
	lastModifyTime[24] = '\0';
	strncpy(path, "root/", 25);

	isDir = _isDir;
	sector_num = sector;
    numBytes = fileSize;
    
	if (fileSize == -1)
		numSectors = 2;
	else
		numSectors  = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors)
	return FALSE;		// not enough space

	if (numSectors <= 2) {
	    for (int i = 0; i < numSectors; i++)
			dataSectors[i] = freeMap->Find();
	}
	else {
		for (int i = 0; i < 3; i++)
			dataSectors[i] = freeMap->Find();
		
		int directBlocks[32];
		for (int i = 0; i < numSectors - 2; i++)
			directBlocks[i] = freeMap->Find();

		synchDisk->WriteSector(dataSectors[2], (char*)directBlocks);	
	}

    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
	if (numSectors <= 2) {
	    for (int i = 0; i < numSectors; i++) {
			ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
			freeMap->Clear((int) dataSectors[i]);
   		}
	}
	else {
		int *directBlocks = new int[32];
		
		synchDisk->ReadSector(dataSectors[2], (char*)directBlocks);
		for (int i = 0; i < numSectors - 2; i++) {
			ASSERT(freeMap->Test((int) directBlocks[i]));
			freeMap->Clear((int) directBlocks[i]);
		}

		for (int i = 0; i < 2; i++) {
			ASSERT(freeMap->Test((int) dataSectors[i]));
			freeMap->Clear((int) dataSectors[i]);
		}

	}
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
	if (offset < 256)
	    return(dataSectors[offset / SectorSize]);
	else {
		int idx = (offset - 256) / SectorSize;
		int *directBlocks = new int[32];

		synchDisk->ReadSector(dataSectors[2], (char*)directBlocks);

		return directBlocks[idx];
	}
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];
	int *directBlocks = new int[32];

	printf("sector_num: %d\n", sector_num);
	printf("type: %s\n", type);
	printf("createTime: %s\n", createTime);
	printf("lastAccessTime: %s\n", lastAccessTime);
	printf("lastModifyTime: %s\n", lastModifyTime);
	printf("path: %s\n", path);

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);

	if (numSectors <= 2) {
		for (i = 0; i < numSectors; i++)
			printf("%d ", dataSectors[i]);
	}
	else {
		for (i = 0; i < 2; i++)
			printf("%d ", dataSectors[i]);
		printf("\nindirect block %d: ", dataSectors[2]);
		
		synchDisk->ReadSector(dataSectors[2], (char*)directBlocks);
		for (i = 0; i < numSectors - 2; i++)
			printf("%d ", directBlocks[i]);
	}

    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
		if (i < 2)
			synchDisk->ReadSector(dataSectors[i], data);
		else 
			synchDisk->ReadSector(directBlocks[i-2], data);

        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	   		if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
				printf("%c", data[j]);
            else
				printf("\\%x", (unsigned char)data[j]);
		}
        printf("\n"); 
    }
    delete [] data;
}
