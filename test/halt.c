/* halt.c
 *	Simple program to test whether running a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS.
 *
 * 	NOTE: for some reason, user programs with global data structures 
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as 
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics!
 */

#include "syscall.h"

int fd1, fd2;
int result;
char buffer[20];

/*int A[500];
int a = 1, b = 1, c = 1;
*/
int
main()
{
	Create("write.txt");
	fd1 = Open("read.txt");
	fd2 = Open("write.txt");
	result = Read(buffer, 20, fd1);
	Write(buffer, result, fd2);
	Close(fd1);
	Close(fd2);
		
/*	int i;
	for (i = 0; i < 500; i++)
		A[i] = 1;
	for (i = 0; i < 500; i++)
		A[i] = 2;
*/  Halt();
    /* not reached */
}
