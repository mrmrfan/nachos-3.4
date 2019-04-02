// list.h 
//	Data structures to manage LISP-like lists.  
//
//      As in LISP, a list can contain any type of data structure
//	as an item on the list: thread control blocks, 
//	pending interrupts, etc.  That is why each item is a "void *",
//	or in other words, a "pointers to anything".
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef LIST_H
#define LIST_H

#include "copyright.h"
#include "utility.h"

// The following class defines a "list element" -- which is
// used to keep track of one item on a list.  It is equivalent to a
// LISP cell, with a "car" ("next") pointing to the next element on the list,
// and a "cdr" ("item") pointing to the item on the list.
//
// Internal data structures kept public so that List operations can
// access them directly.

class ListElement {
   public:
     ListElement(void *itemPtr, int sortKey);	// initialize a list element

     ListElement *next;		// next element on list, 
				// NULL if this is the last
     int key;		    	// priority, for a sorted list
     void *item; 	    	// pointer to item on the list
};

// The following class defines a "list" -- a singly linked list of
// list elements, each of which points to a single item on the list.
//
// By using the "Sorted" functions, the list can be kept in sorted
// in increasing order by "key" in ListElement.

class List {
  public:
    List();			// initialize the list
    ~List();			// de-allocate the list

    void Prepend(void *item); 	// Put item at the beginning of the list
    void Append(void *item); 	// Put item at the end of the list
    void *Remove(); 	 	// Take item off the front of the list

    void Remove(void *item);    // Remove specific item from list

    void Mapcar(VoidFunctionPtr func);	// Apply "func" to every element 
					// on the list
    unsigned int NumInList() { return numInList;};

    bool IsEmpty();		// is the list empty? 
    

    // Routines to put/get items on/off list in order (sorted by key)
    void SortedInsert(void *item, int sortKey);	// Put item into list
    void *SortedRemove(int *keyPtr); 	  	// Remove first item from list

  private:
    ListElement *first;  	// Head of the list, NULL if list is empty
    ListElement *last;		// Last element of list
    int numInList;		// number of elements in list
};

class myListElem {
  public:
	myListElem(int _val);
	myListElem* next;
	int val;
};

class myList {
 public:
    myList();             // initialize
    ~myList();            // deconstruct
    void Prepend(int a);       // add the int to the front
    void Append(int a);        // add the int to the end
	void Remove(int a);   // remove a certain int
	int Remove();        // remove and return the int in the front
	bool IsEmpty();      // whether the list is empty or not
				  
 private:
	myListElem* first;
	myListElem* last;
	int elemNum;
};

#endif // LIST_H
