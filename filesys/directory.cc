// directory.cc 
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
	table[i].inUse = FALSE;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{ 
    delete [] table;
} 

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
Directory::FetchFrom(OpenFile *file)
{
    (void) file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
Directory::WriteBack(OpenFile *file)
{
    (void) file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::FindIndex(char *name)
{
	char *Name = new char[20];

    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse) {
			fileSystem->nameFile->ReadAt(Name, table[i].nameLen, table[i].namePos);
			if (!strncmp(Name, name, 20))
				return i;
		}

	delete Name;
    return -1;		// name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't 
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::Find(char *name)
{
    int i, j;
	int pos;
	char *Name = new char[24];
	
	pos = -1;
	for (j = 0; name[j] != NULL; j++)
		if (name[j] == '/')
			pos = j;
	for (j = 0; name[j+pos+1] != NULL; j++)
		Name[j] = name[j+pos+1];
	Name[j] = '\0';

	i = FindIndex(Name);
	delete Name;

    if (i != -1)
	return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool
Directory::Add(char *name, int newSector)
{
	int nameFilePos;
    int j;
	int pos;
	char *Name = new char[24];
	
	pos = -1;
	for (j = 0; name[j] != NULL; j++)	
		if (name[j] == '/')
			pos = j;
	for (j = 0; name[j+pos+1] != NULL; j++)
	    Name[j] = name[j+pos+1];
	Name[j] = '\0';

    if (FindIndex(Name) != -1) {
		delete Name;
		return FALSE;
	}

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse) {
            table[i].inUse = TRUE;

			fileSystem->nameFile->ReadAt((char*)&nameFilePos, (int)(sizeof(int)), 0);
			table[i].namePos = nameFilePos;
			table[i].nameLen = 20;
			nameFilePos += 20;
			fileSystem->nameFile->WriteAt((char*)&nameFilePos, (int)(sizeof(int)), 0);
			fileSystem->nameFile->WriteAt(Name, table[i].nameLen, table[i].namePos);
            table[i].sector = newSector;
			delete Name;
        return TRUE;
	}
	delete Name;
    return FALSE;	// no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool
Directory::Remove(char *name)
{ 
    int i, j;
	int pos;
	char *Name = new char[24];

	pos = -1;
	for (j = 0; name[j] != NULL; j++)
		if (name[j] == '/')
			pos = j;
	for (j = 0; name[j+pos+1] != NULL; j++)
		Name[j] = name[j+pos+1];
	Name[j] = '\0';
	
	i = FindIndex(Name);
	delete Name;

    if (i == -1)
	return FALSE; 		// name not in directory
    table[i].inUse = FALSE;
    return TRUE;	
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 
//----------------------------------------------------------------------

void
Directory::List()
{
   char *Name = new char[20];
   for (int i = 0; i < tableSize; i++)
	if (table[i].inUse) {
	    fileSystem->nameFile->ReadAt(Name, table[i].nameLen, table[i].namePos);
		printf("Name: %s\n", Name);
	}


   delete Name;
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void
Directory::Print()
{ 
    FileHeader *hdr = new FileHeader;
	char *Name = new char[20];

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
	if (table[i].inUse) {
		fileSystem->nameFile->ReadAt(Name, table[i].nameLen, table[i].namePos);

	    printf("Name: %s, Sector: %d\n", Name, table[i].sector);
	    hdr->FetchFrom(table[i].sector);
	    hdr->Print();
	}
    printf("\n");
    delete hdr;
}

int
Directory::findDir(char *name)
{
	int pos;
	int i, j;
	int dirSector;
	char *dir = new char[24];
	int p;
	Directory *directory = new Directory(10);
	OpenFile *dirFile = new OpenFile(1);

	pos = 0;
	for (i = 0; name[i] != NULL; i++)
		if (name[i] == '/')
			pos = i;
	
	if (pos == 0) {
		delete dir;
		return 1;
	}
	
	directory->FetchFrom(dirFile);
	delete dirFile;

	i = 0;
	p = 0;
	while(i <= pos) {
		if (name[i] != '/') {
			dir[p++] = name[i];
		}
		else {
			dir[p] = '\0';
			p = 0;
			printf("dir name: %s\n", dir);		
			dirSector = directory->Find(dir);
			if (dirSector == -1) {
				break;
			}
			dirFile = new OpenFile(dirSector);
			directory->FetchFrom(dirFile);
			delete dirFile;
		}
		i++;
	}
	
	delete dir;
	delete directory;
	return dirSector;
}
